import adversary.analyze_common as common

class FCFSAnalyzer(common.BaseAnalyzer):
    """Performs analysis for FCFS scheduling."""
    def __init__(self, config_fp: str, log_fp: str):
        super().__init__() # Init base class
        self._analyze_log(log_fp) # Perform analysis
        self._parse_config(config_fp) # Parse config file

    @staticmethod
    def get_policy_name() -> str:
        """Returns the policy name."""
        return "fcfs"

    def _analyze_log_packet(self, packet: common.Packet) -> None:
        """Implementation-specific parsing."""
        return

    def get_optimal_strategy(
        self, r_I_gbps: float, r_A_gbps: float) -> common.AttackStrategy:
        """Given a configuration, returns a tuple representing
           the adversary's optimal strategy for the policy."""
        total_adversarial_work = ((r_A_gbps / common.Packet.MIN_PACKET_SIZE) *
                                  self.maximum_attack_jsize)

        total_innocent_work = ((r_I_gbps / self.average_innocent_psize) *
                               self.average_innocent_jsize)

        # Compute the theoretical goodput
        expected_goodput = r_I_gbps * min(
            1, 1 / (total_innocent_work + total_adversarial_work))

        # Compute the theoretical DF
        ideal_goodput = min(r_I_gbps, self.r_max)
        alpha = (0 if (r_A_gbps == 0) else max(0,
                    (ideal_goodput - expected_goodput)) / r_A_gbps)

        # Return the optimal strategy
        return common.AttackStrategy(
                self.maximum_attack_jsize, self.maximum_attack_jsize,
                common.Packet.MIN_PACKET_SIZE, expected_goodput, alpha)
