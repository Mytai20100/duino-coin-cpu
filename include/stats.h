#ifndef STATS_H
#define STATS_H

#include <string>
#include <atomic>
#include <chrono>

struct SystemStats {
    std::chrono::steady_clock::time_point start_time;
    std::atomic<unsigned long> shares_sent{0};
    std::atomic<unsigned long> data_bytes_sent{0};
    std::atomic<unsigned long> data_bytes_received{0};
    
    SystemStats() : start_time(std::chrono::steady_clock::now()) {}
    
    std::string get_uptime() const;
    double get_cpu_usage() const;
    std::string get_cpu_name() const;
};

class StatsDisplay {
public:
    static void show_stats(const SystemStats& sys_stats,
                          unsigned long accepted,
                          unsigned long rejected,
                          const std::string& pool_address,
                          int pool_port);
    
private:
    static std::string format_bytes(unsigned long bytes);
    static std::string format_uptime(long seconds);
};

#endif