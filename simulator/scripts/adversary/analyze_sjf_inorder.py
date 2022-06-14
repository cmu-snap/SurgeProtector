import adversary.analyze_common as common

from collections import defaultdict

class FlowMetrics:
    """Flow-level average job and packet sizes."""
    def __init__(self):
        self.num_packets = 0
        self.total_psize = 0
        self.total_jsize = 0

    def add_packet(self, psize: int, jsize: float) -> None:
        """Registers a new packet to this flow."""
        self.num_packets += 1
        self.total_psize += psize
        self.total_jsize += jsize

    def get_average_jsize(self) -> float:
        """Returns the average job size for this flow."""
        return (self.total_jsize / self.num_packets)

class SJFInorderAnalyzer(common.BaseAnalyzer):
    """Performs analysis for SJF-Inorder scheduling."""
    def __init__(self, config_fp: str, log_fp: str):
        super().__init__() # Init base class
        self.flows_sorted = None # List of FlowMetrics sorted by average jsize
        self.jsize_expectations = dict() # jsize -> cumulative jsize expectation
        self.flow_metrics_map = defaultdict(FlowMetrics) # Flow ID -> FlowMetrics

        self._analyze_log(log_fp) # Perform analysis
        self._parse_config(config_fp) # Parse config file
        self._compute_expectation() # Populate the jsize_expectations map

    @staticmethod
    def get_policy_name() -> str:
        """Returns the policy name."""
        return "sjf_inorder"

    def _analyze_log_packet(self, packet: common.Packet) -> None:
        """Implementation-specific parsing."""
        # Add the packet to the corresponding flow
        self.flow_metrics_map[packet.flow_id].add_packet(
            packet.metrics.psize, packet.metrics.jsize)

    def _compute_expectation(self) -> None:
        """Populates the cumulative expectations map."""
        # Sort flows in increasing order of average job size
        self.flows_sorted = [value for value in sorted(
                             self.flow_metrics_map.values(),
                             key=lambda metrics: metrics.get_average_jsize())]

        # Construct dictionary mapping average jsize to the
        # expected job size for all flows with that average.
        jsize_cumulative_expectation = 0
        for flow in self.flows_sorted:
            jsize_cumulative_expectation += (
                flow.total_jsize / self.num_total_packets)

            self.jsize_expectations[int(
                flow.get_average_jsize())] = jsize_cumulative_expectation

    def get_optimal_strategy(
        self, r_I_gbps: float, r_A_gbps: float) -> common.AttackStrategy:
        """Given a configuration, returns a tuple representing
           the adversary's optimal strategy for the policy."""
        optimal_jsize = self.maximum_innocent_jsize
        for jsize, expectation in sorted(self.jsize_expectations.items()):
            total_adversarial_work = ((r_A_gbps / common.Packet.MIN_PACKET_SIZE) *
                                      max(0, jsize - 1))

            total_innocent_work = ((r_I_gbps / self.average_innocent_psize) *
                                   expectation)

            # Current choice of J_{A} pushes the system over capacity
            if (total_innocent_work + total_adversarial_work >= 1):
                optimal_jsize = (jsize - 1)
                break

        # Compute the theoretical goodput
        cumulative_scheduled_psize = 0
        for flow in self.flows_sorted:
            flow_jsize = flow.get_average_jsize()
            if (flow_jsize > optimal_jsize): break
            cumulative_scheduled_psize += flow.total_psize

        expected_goodput = ((r_I_gbps * cumulative_scheduled_psize) /
                            (self.num_total_packets * self.average_innocent_psize))

        # Compute the theoretical DF
        ideal_goodput = min(r_I_gbps, self.r_max)
        alpha = (0 if (r_A_gbps == 0) else max(0,
                    (ideal_goodput - expected_goodput)) / r_A_gbps)

        # Return the optimal strategy
        return common.AttackStrategy(
            optimal_jsize, optimal_jsize,
            common.Packet.MIN_PACKET_SIZE, expected_goodput, alpha)
