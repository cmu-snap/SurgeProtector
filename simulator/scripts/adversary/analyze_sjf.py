import adversary.analyze_common as common

from collections import defaultdict

class SJFAnalyzer(common.BaseAnalyzer):
    """Performs analysis for SJF scheduling."""
    def __init__(self, config_fp: str, log_fp: str):
        super().__init__() # Init base class
        self.arrivals = [] # List of arrival metrics sorted by jsize
        self.jsizes_frequencies = defaultdict(int) # jsize -> frequency
        self.jsize_expectations = dict() # jsize -> cumulative expectation

        self._analyze_log(log_fp) # Perform analysis
        self._parse_config(config_fp) # Parse config file
        self._compute_expectation() # Populate the jsize_expectations map

    @staticmethod
    def get_policy_name() -> str:
        """Returns the policy name."""
        return "sjf"

    def _analyze_log_packet(self, packet: common.Packet) -> None:
        """Implementation-specific parsing."""
        self.arrivals.append(packet.metrics)
        self.jsizes_frequencies[int(packet.metrics.jsize)] += 1

    def _compute_expectation(self) -> None:
        """Populates the cumulative expectations map."""
        # Sort arrivals in increasing order of job size
        self.arrivals.sort(key=lambda x: x.jsize)

        # Compute the pdf and expectation
        jsize_cumulative_expectation = 0
        for jsize, frequency in sorted(self.jsizes_frequencies.items()):
            density = (frequency / self.num_total_packets)
            jsize_cumulative_expectation += (density * jsize)
            self.jsize_expectations[jsize] = jsize_cumulative_expectation

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
        for arrival_metrics in self.arrivals:
            if (arrival_metrics.jsize > optimal_jsize): break
            cumulative_scheduled_psize += arrival_metrics.psize

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
