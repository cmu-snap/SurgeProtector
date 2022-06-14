from __future__ import annotations
from abc import ABC, abstractmethod
import io, libconf

class SchedulingMetrics:
    """Metrics used for scheduling."""
    def __init__(self, psize: int, jsize: float):
        self.jsize = jsize # Corresponding job size
        self.psize = psize # Corresponding packet size
        self.z_ratio = (jsize / psize) # {Job/Packet} size

class Packet:
    """Represents a packet (from an output log)."""
    # Constant parameters
    MIN_PACKET_SIZE = 512 # 64B
    MAX_PACKET_SIZE = 12144 # 1518B

    def __init__(self, arrive_clk: float, depart_clk: float,
                 flow_id: str, class_tag: str, psize: int,
                 jsize: float):

        self.arrive_clk = arrive_clk # Arrival clock
        self.depart_clk = depart_clk # Departure clock
        self.flow_id = flow_id # This packet's flow ID
        self.class_tag = class_tag # The traffic class
        self.metrics = SchedulingMetrics(psize, jsize)

    @staticmethod
    def from_log(line: str) -> Packet:
        """Parse a line of packet log (simulator output)."""
        values = [x.strip() for x in line.split(";")]
        assert len(values) >= 6 # Sanity check

        return Packet(float(values[0]), # Arrival clk
                      float(values[1]), # Depart clk
                      values[2],        # Flow ID
                      values[3],        # Class tag
                      int(values[4]),   # Packet size
                      float(values[5])) # Job size (estimate)

class AttackStrategy:
    """Represents an adversary's attack strategy."""
    def __init__(self, estimated_jsize: float, actual_jsize: float,
                 psize: int, expected_goodput: float, alpha: float):
        self.estimated_jsize = estimated_jsize # Estimated job size (ns)
        self.actual_jsize = actual_jsize # Actual job size (ns)
        self.psize = psize # Packet size (bits)

        self.expected_goodput_gbps = expected_goodput # Theoretical goodput (Gbps)
        self.expected_alpha = alpha # Theoretical displacement factor

class BaseAnalyzer(ABC):
    """Abstract base class for analyzing policies
    from the perspective of an optimal adversary."""
    def __init__(self):
        # Useful top-level statistics
        self.r_max = 0
        self.num_total_packets = 0
        self.average_innocent_psize = 0
        self.maximum_innocent_psize = 0
        self.average_innocent_jsize = 0
        self.maximum_innocent_jsize = 0

        # Config parameters
        self.maximum_attack_jsize = 0

    @abstractmethod
    def get_policy_name(self) -> str:
        """Returns the policy name."""
        raise NotImplementedError

    @abstractmethod
    def _analyze_log_packet(self, packet: Packet) -> None:
        """Implementation-specific packet analysis."""
        raise NotImplementedError

    @abstractmethod
    def get_optimal_strategy(
        self, r_I_gbps: float, r_A_gbps: float) -> AttackStrategy:
        """Given a configuration, returns a tuple representing
           the adversary's optimal strategy for the policy."""
        raise NotImplementedError

    def _analyze_log(self, log_fp: str) -> None:
        """Analyzes the given log file."""
        with open(log_fp, 'r') as lines:
            for line in lines:
                packet = Packet.from_log(line)
                psize = packet.metrics.psize
                jsize = packet.metrics.jsize

                # Update statistics
                self.num_total_packets += 1
                self.average_innocent_psize += psize
                self.average_innocent_jsize += jsize
                self.maximum_innocent_psize = max(
                    self.maximum_innocent_psize, psize)

                self.maximum_innocent_jsize = max(
                    self.maximum_innocent_jsize, jsize)

                # Invoke the specific parsing functionality
                self._analyze_log_packet(packet)

        # Compute the average packet and job size, and r_Max
        self.average_innocent_psize /= self.num_total_packets
        self.average_innocent_jsize /= self.num_total_packets
        self.r_max = (self.average_innocent_psize /
                      self.average_innocent_jsize)

        # Convert the packet size to an integer
        self.average_innocent_psize = int(
            self.average_innocent_psize)

        # Print stats
        if (False):
            print("Average job size: {:.2f} ns".format(
                self.average_innocent_jsize))

            print("Maximum job size: {} ns".format(
                self.maximum_innocent_jsize))

            print("Average packet size: {} bits".format(
                self.average_innocent_psize))

            print("Maximum packet size: {} bits\n".format(
                self.maximum_innocent_psize))

    def _parse_config(self, config_fp: str) -> None:
        """Parses the given configuration file. Note that this
        method should always be invoked AFTER analyze_log."""
        with io.open(config_fp) as f:
            config = libconf.load(f)

            # The maximum attack job size (ns)
            # is specified in the config file.
            self.maximum_attack_jsize = (config[
                "application"]["max_attack_job_size_ns"])
