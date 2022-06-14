import adversary.analyze_common as common

from collections import defaultdict
from fractions import Fraction
import numpy as np

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

    def get_average_z_ratio(self) -> float:
        """Returns the average z-ratio for this flow."""
        return (self.total_jsize / self.total_psize)

class WSJFInorderAnalyzer(common.BaseAnalyzer):
    """Performs analysis for WSJF-Inorder scheduling."""
    def __init__(self, config_fp: str, log_fp: str):
        super().__init__() # Init base class
        self.flows_sorted = None # List of FlowMetrics sorted by z-ratio
        self.zs_expectation_jsizes = None # Cumulative jsize expectation
        self.flow_metrics_map = defaultdict(FlowMetrics) # Flow ID -> FlowMetrics

        # Parameters
        self.SCALING_FACTOR = 4096
        self.OPTIMAL_ADV_PSIZE = 8192

        # Perform analysis
        self._analyze_log(log_fp) # Perform analysis
        self._parse_config(config_fp) # Parse config file
        self._compute_expectation() # Populate jsize expectations

    @staticmethod
    def get_policy_name() -> str:
        """Returns the policy name."""
        return "wsjf_inorder"

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
                             key=lambda metrics: metrics.get_average_z_ratio())]

        # Compute the max z-ratio
        max_jp_ratio = self.flows_sorted[-1].get_average_z_ratio()
        self.zs_expectation_jsizes = np.zeros(round(self.SCALING_FACTOR *
                                                    max_jp_ratio) + 1)

        # Compute the cumulative expectation
        jsize_cumulative_expectation = 0
        for flow in self.flows_sorted:
            jsize_cumulative_expectation += (
                flow.total_jsize / self.num_total_packets)

            # Compute the scaled z-ratio and update the expectation
            scaled_jp_ratio = round(flow.get_average_z_ratio() *
                                    self.SCALING_FACTOR)

            self.zs_expectation_jsizes[scaled_jp_ratio] = (
                jsize_cumulative_expectation)

    def get_optimal_strategy(
        self, r_I_gbps: float, r_A_gbps: float) -> common.AttackStrategy:
        """Given a configuration, returns a tuple representing
           the adversary's optimal strategy for the policy."""
        optimal_z = (self.maximum_innocent_jsize /
                     common.Packet.MIN_PACKET_SIZE)
        current_z = 0
        for expectation in self.zs_expectation_jsizes:
            total_adversarial_work = (r_A_gbps * current_z)
            total_innocent_work = ((r_I_gbps / self.average_innocent_psize) *
                                   expectation)

            # Current choice of Z_{A} pushes the system over capacity
            if (total_innocent_work + total_adversarial_work >= 1): break
            optimal_z = current_z
            current_z += (1 / self.SCALING_FACTOR)

        # Compute the theoretical goodput
        cumulative_scheduled_psize = 0
        for flow in self.flows_sorted:
            flow_z = flow.get_average_z_ratio()
            if (flow_z > optimal_z): break
            cumulative_scheduled_psize += flow.total_psize

        expected_goodput = ((r_I_gbps * cumulative_scheduled_psize) /
                            (self.num_total_packets * self.average_innocent_psize))

        # Compute the theoretical DF
        ideal_goodput = min(r_I_gbps, self.r_max)
        alpha = (0 if (r_A_gbps == 0) else max(0,
                    (ideal_goodput - expected_goodput)) / r_A_gbps)

        # Compute the ratio and rescale the denominator (psize) to be sufficiently large
        ratio = Fraction(optimal_z).limit_denominator(common.Packet.MAX_PACKET_SIZE)
        (numerator, denominator) = (ratio.numerator, ratio.denominator)
        if denominator < self.OPTIMAL_ADV_PSIZE:
            numerator *= (self.OPTIMAL_ADV_PSIZE / denominator)
            denominator = self.OPTIMAL_ADV_PSIZE

        # Return the optimal strategy
        return common.AttackStrategy(
            int(numerator), int(numerator),
            denominator, expected_goodput, alpha)
