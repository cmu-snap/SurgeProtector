import adversary.analyze_common as common

from fractions import Fraction
import numpy as np

class WSJFAnalyzer(common.BaseAnalyzer):
    """Performs analysis for WSJF scheduling."""
    def __init__(self, config_fp: str, log_fp: str):
        super().__init__() # Init base class
        self.arrivals = [] # List of arrival metrics sorted by z-ratio
        self.zs_expectation_jsizes = None # Cumulative jsize expectation

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
        return "wsjf"

    def _analyze_log_packet(self, packet: common.Packet) -> None:
        """Implementation-specific parsing."""
        # Update the arrivals list
        self.arrivals.append(packet.metrics)

    def _compute_expectation(self) -> None:
        """Populates the cumulative expectations map."""
        # Sort arrivals in increasing order of z
        self.arrivals.sort(key=lambda x: x.z_ratio)

        # Compute the max z-ratio
        max_z_ratio = self.arrivals[-1].z_ratio
        self.zs_expectation_jsizes = np.zeros(round(self.SCALING_FACTOR *
                                                    max_z_ratio) + 1)
        # Compute the cumulative expectation
        jsize_cumulative_expectation = 0
        for arrival_metrics in self.arrivals:
            jsize_cumulative_expectation += (
                arrival_metrics.jsize / self.num_total_packets)

            # Compute the scaled z-ratio and update the expectation
            scaled_z_ratio = round(arrival_metrics.z_ratio *
                                   self.SCALING_FACTOR)

            self.zs_expectation_jsizes[scaled_z_ratio] = (
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
            total_innocent_work = ((r_I_gbps / self.average_innocent_psize) * expectation)

            # Current choice of Z_{A} pushes the system over capacity
            if (total_innocent_work + total_adversarial_work >= 1): break
            optimal_z = current_z
            current_z += (1 / self.SCALING_FACTOR)

        # Compute the theoretical goodput
        cumulative_scheduled_psize = 0
        for arrival_metrics in self.arrivals:
            z = arrival_metrics.z_ratio
            if (z > optimal_z): break
            cumulative_scheduled_psize += arrival_metrics.psize

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
