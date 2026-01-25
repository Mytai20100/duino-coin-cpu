#ifndef LOGGER_H
#define LOGGER_H

#include <string>

class Logger {
public:
    static void enable();
    static void disable();
    static bool is_enabled();
    
    static void info(const std::string& message);
    static void success(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    
    // XMRig-style network logging
    static void net_connect(const std::string& pool, int port);
    static void net_connected(const std::string& version, int ping);
    static void net_job(int thread_id, int difficulty, const std::string& algo);
    static void net_accepted(int thread_id, unsigned long accepted, unsigned long rejected, 
                            double hashrate, double total_hashrate, double time, int ping);
    static void net_rejected(int thread_id, unsigned long accepted, unsigned long rejected,
                            const std::string& reason, int ping);
    static void net_block(int thread_id, unsigned long blocks);
    static void net_error(const std::string& message);
    static void net_disconnected(const std::string& reason);
    
    // Mining stats
    static void share(int thread_id, const std::string& result_type, 
                     unsigned long accepted, unsigned long rejected,
                     double hashrate, double total_hashrate, 
                     double time, int difficulty, int ping);
    
    static void speed(const std::string& hashrate);
    static void speed(int threads, double total_hashrate,
                     unsigned long accepted, unsigned long rejected);
    
    // Speed update - prints new line instead of overwriting
    static void speed_update(int threads, double total_hashrate,
                            unsigned long accepted, unsigned long rejected);
    
    // Helper functions
    static std::string format_hashrate(double hashrate);
    static std::string format_difficulty(int difficulty);
    static std::string get_colored_box(const std::string& type);
    
    // XMRig-style info display
    static void print_versions(const std::string& app_version, const std::string& libuv_version);
    static void print_cpu_info(const std::string& brand, int threads, const std::string& arch);
    static void print_pool_info(const std::string& pool, int port, const std::string& user);
    static void print_commands();
    static void print_separator();
    static void print_header();
    
    // Benchmark
    static void benchmark_start(int threads);
    static void benchmark_result(unsigned long total_hashes, double duration, 
                                double total_hashrate, double per_thread);
    
    // CPU summary
    static void cpu_summary(int total_threads, double total_hashrate);
};

#endif