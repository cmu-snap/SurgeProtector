from typing import Union

class RateConverter:
    """Implements helper functions to convert between human-
    readable rates (e.g., 10K) and numerical values (10000)."""

    @staticmethod
    def from_str(rate_str: Union[int, str]) -> int:
        """Helper function to parse a rate string (e.g., '1K')."""
        rate_map = {'K': 10 ** 3,
                    'M': 10 ** 6,
                    'G': 10 ** 9}

        assert rate_str != ""
        if isinstance(rate_str, str) and rate_str[-1] in rate_map.keys():
            return int(float(rate_str[:-1]) * rate_map[rate_str[-1]])

        else: return int(rate_str)

    @staticmethod
    def to_str(rate: Union[int, str], pretty=False) -> str:
        """Helper function to get a standardized string representation of
        a given rate or rate string. (e.g., 1000000 or '1000K' -> 1M)."""
        rate = RateConverter.from_str(rate)
        rate_str = str(rate)

        denominations = [(10 ** 9, 'G'),
                         (10 ** 6, 'M'),
                         (10 ** 3, 'K')]

        for (num, suffix) in denominations:
            if (rate >= num) and (rate % num == 0):
                rate_str = "{}{}{}".format(int(rate / num),
                    " " if pretty else "", suffix)
                break

        return rate_str
