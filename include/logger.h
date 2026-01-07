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
    static void share(int thread_id, const std::string& status, 
                     unsigned long accepted, unsigned long rejected,
                     double hashrate, double total_hashrate,
                     double compute_time, int difficulty, int ping);
    
    // Helper functions for formatting
    static std::string format_hashrate(double hashrate);
    static std::string format_difficulty(int difficulty);
};

#endif