#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

// STD headers
#include <string>
#include <vector>

/**
 * Represents a min-heap entry.
 */
template<class Tag, class Metric> class MinHeapEntry {
private:
    Tag tag_; // Tag corresponding to this entry
    double insert_time_; // Time (in ns) of insertion
    Metric primary_metric_; // The primary priority metric

public:
    explicit MinHeapEntry(const Tag& tag, const Metric& metric, const double in_time=0) :
                          tag_(tag), insert_time_(in_time), primary_metric_(metric) {}
    // Accessors
    const Tag& tag() const { return tag_; }
    double getInsertTime() const { return insert_time_; }
    Metric getPrimaryMetric() const { return primary_metric_; }

    // Comparator
    bool operator<(const MinHeapEntry& other) const {
        // Sort by the primary metric
        if (primary_metric_ != other.primary_metric_) {
            return primary_metric_ > other.primary_metric_;
        }
        // If tied, sort by insertion time
        return insert_time_ >= other.insert_time_;
    }
};

/**
 * Helper function. Given a packet rate and expected packet size,
 * returns the inter-arrival time for the corresponding traffic.
 */
double getTrafficInterArrivalTimeInNs(
    const double traffic_rate_in_bits_per_sec,
    const uint32_t expected_packet_size_in_bits);

/**
 * Helper string parsing functions. Emulates Python's str.split().
 */
bool endsWith(const std::string& s, const std::string& suffix);
std::vector<std::string> split(const std::string& s,
                               const std::string& delimiter);

/**
 * Helper function. Returns whether a and b are equal
 * (within floating-point error margin).
 */
bool DoubleApproxEqual(const double a, const double b,
                       const double epsilon=1e-6);

#endif // COMMON_UTILS_H
