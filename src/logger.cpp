#include "../include/logger.h"
#include "../include/config.h"
#include <thread>
#include <csignal>   
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cmath>
#include <atomic>  
#include <fstream>
#include <sys/sysinfo.h>

static bool enabled = true;
static std::mutex log_mutex;

// Color codes - XMRig style
#define RESET         "\033[0m"
#define BLACK         "\033[30m"
#define RED           "\033[31m"
#define GREEN         "\033[32m"
#define YELLOW        "\033[33m"
#define BLUE          "\033[34m"
#define MAGENTA       "\033[35m"
#define CYAN          "\033[36m"
#define WHITE         "\033[37m"
#define GRAY          "\033[90m"
#define BRIGHT_GREEN  "\033[92m"
#define BRIGHT_BLUE   "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define CHARTREUSE    "\033[38;2;127;255;0m"  

// Text styles
#define BOLD          "\033[1m"
#define DIM           "\033[2m"

// Background colors for tags
#define BG_BLUE       "\033[44m"
#define BG_MAGENTA    "\033[45m"
#define BG_CYAN       "\033[46m"
#define BG_GREEN      "\033[42m"
#define BG_RED        "\033[41m"
#define BG_YELLOW     "\033[43m"

#define TAG_NET       BG_BLUE WHITE "  net   " RESET           
#define TAG_CPU       BG_CYAN BLACK "  cpu   " RESET         
#define TAG_MINER     BG_MAGENTA WHITE " miner  " RESET       

static std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << GRAY << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d") << "]" << RESET;
    return ss.str();
}

std::string Logger::get_colored_box(const std::string& type) {
    if (type == "net") return TAG_NET;
    if (type == "cpu") return TAG_CPU;
    if (type == "miner") return TAG_MINER;
    return " ";
}

std::string Logger::format_hashrate(double hashrate) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);
    
    if (hashrate >= 1000000000.0) {
        ss << (hashrate / 1000000000.0) << " GH/s";
    } else if (hashrate >= 1000000.0) {
        ss << (hashrate / 1000000.0) << " MH/s";
    } else if (hashrate >= 1000.0) {
        ss << (hashrate / 1000.0) << " kH/s";
    } else {
        ss << hashrate << " H/s";
    }
    
    return ss.str();
}

std::string Logger::format_difficulty(int difficulty) {
    std::stringstream ss;
    
    if (difficulty >= 1000000) {
        int rounded = (difficulty + 500000) / 1000000;
        ss << rounded << "M";
    } else if (difficulty >= 1000) {
        int rounded = (difficulty + 500) / 1000;
        ss << rounded << "k";
    } else {
        ss << difficulty;
    }
    
    return ss.str();
}

void Logger::enable() { enabled = true; }
void Logger::disable() { enabled = false; }
bool Logger::is_enabled() { return enabled; }

void Logger::info(const std::string& message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_NET << " " << WHITE << message << RESET << "\n" << std::flush;
}

void Logger::success(const std::string& message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_CPU << " " << BRIGHT_GREEN << message << RESET << "\n" << std::flush;
}

void Logger::warning(const std::string& message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_NET << " " << YELLOW << message << RESET << "\n" << std::flush;
}

void Logger::error(const std::string& message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_NET << " " << RED << message << RESET << "\n" << std::flush;
}

void Logger::net_connect(const std::string& pool, int port) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_NET << " " 
              << WHITE << "use pool " << CYAN << pool << ":" << port << RESET << "\n" 
              << std::flush;
}

void Logger::net_connected(const std::string& version, int ping) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_NET << " " 
              << BRIGHT_MAGENTA << "new job from " << CYAN << "pool" << RESET
              << WHITE << " diff " << CYAN << "20M" << RESET 
              << WHITE << " algo " << CYAN << "DUCOS1" << RESET 
              << WHITE << " height " << CYAN << version << RESET << "\n"
              << std::flush;
}

void Logger::net_job(int thread_id, int difficulty, const std::string& algo) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_MINER << " " 
              << WHITE << "new job diff " << CYAN << format_difficulty(difficulty) << RESET 
              << WHITE << " algo " << CYAN << algo << RESET << "\n" << std::flush;
}

void Logger::net_accepted(int thread_id, unsigned long accepted, unsigned long rejected, 
                         double hashrate, double total_hashrate, double time, int ping) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    
    double accept_rate = (accepted + rejected) > 0 ? 
        (accepted * 100.0 / (accepted + rejected)) : 100.0;
    
    std::cout << get_timestamp() << " " << TAG_CPU << " " 
              << BRIGHT_GREEN << "accepted" << RESET
              << BRIGHT_GREEN << " (" << accepted << "/" << (accepted + rejected) 
              << " " << std::fixed << std::setprecision(1) << accept_rate << "%)" << RESET
              << GRAY << " (" << std::fixed << std::setprecision(0) << time * 1000 << " ms)" << RESET 
              << "\n" << std::flush;
}

void Logger::net_rejected(int thread_id, unsigned long accepted, unsigned long rejected,
                         const std::string& reason, int ping) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    
    double accept_rate = (accepted + rejected) > 0 ? 
        (accepted * 100.0 / (accepted + rejected)) : 0.0;
    
    std::cout << get_timestamp() << " " << TAG_CPU << " " 
              << RED << "rejected" << RESET 
              << RED << " (" << accepted << "/" << (accepted + rejected) 
              << " " << std::fixed << std::setprecision(1) << accept_rate << "%)" << RESET
              << GRAY << " " << reason << RESET
              << GRAY << " (" << ping << " ms)" << RESET << "\n" << std::flush;
}

void Logger::net_block(int thread_id, unsigned long blocks) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_CPU << " " 
              << YELLOW << BOLD << "BLOCK FOUND!" << RESET 
              << GRAY << " (total: " << blocks << ")" << RESET << "\n" << std::flush;
}

void Logger::net_error(const std::string& message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_NET << " " 
              << RED << "error: " << message << RESET << "\n" << std::flush;
}

void Logger::net_disconnected(const std::string& reason) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_NET << " " 
              << WHITE << "disconnected: " << GRAY << reason << RESET << "\n" << std::flush;
}

void Logger::share(int thread_id, const std::string& result_type,
                  unsigned long accepted, unsigned long rejected,
                  double hashrate, double total_hashrate,
                  double time, int difficulty, int ping) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    
    double accept_rate = (accepted + rejected) > 0 ? 
        (accepted * 100.0 / (accepted + rejected)) : 100.0;
    
    int actual_diff = difficulty / 100;
    
    if (result_type == "ACCEPT") {
        std::cout << get_timestamp() << " " << TAG_CPU << " " 
                  << CHARTREUSE << "accepted" << RESET
                  << CHARTREUSE << " (" << accepted << "/" << (accepted + rejected) 
                  << " " << std::fixed << std::setprecision(1) << accept_rate << "%)" << RESET
                  << WHITE << " diff " << CYAN << format_difficulty(actual_diff) << RESET
                  << GRAY << " (" << std::fixed << std::setprecision(0) << time * 1000 << " ms)"
                  << " (" << ping << " ms)" << RESET 
                  << "\n" << std::flush;
    } else if (result_type == "REJECT") {
        std::cout << get_timestamp() << " " << TAG_CPU << " " 
                  << RED << "rejected" << RESET 
                  << RED << " (" << accepted << "/" << (accepted + rejected) 
                  << " " << std::fixed << std::setprecision(1) << accept_rate << "%)" << RESET
                  << WHITE << " diff " << CYAN << format_difficulty(actual_diff) << RESET
                  << GRAY << " (" << std::fixed << std::setprecision(0) << time * 1000 << " ms)"
                  << " (" << ping << " ms)" << RESET 
                  << "\n" << std::flush;
    } else if (result_type == "BLOCK") {
        std::cout << get_timestamp() << " " << TAG_CPU << " " 
                  << YELLOW << BOLD << "BLOCK FOUND!" << RESET 
                  << WHITE << " diff " << CYAN << format_difficulty(actual_diff) << RESET
                  << GRAY << " (" << ping << " ms)" << RESET
                  << "\n" << std::flush;
    }
}

void Logger::speed_update(int threads, double total_hashrate,
                         unsigned long accepted, unsigned long rejected) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    
    double hashrate_10s = total_hashrate;
    double hashrate_60s = total_hashrate * 0.98;
    double hashrate_15m = total_hashrate * 0.95;
    
    std::cout << get_timestamp() << " " << TAG_MINER << " "
              << WHITE << "speed " << CYAN << "10s/60s/15m" << RESET << " "
              << CYAN << format_hashrate(hashrate_10s) << RESET << " "
              << CYAN << format_hashrate(hashrate_60s) << RESET << " "
              << CYAN << format_hashrate(hashrate_15m) << RESET 
              << WHITE << " n/a" << RESET
              << WHITE << " max " << CYAN << format_hashrate(total_hashrate * 1.05) << RESET
              << "\n" << std::flush;
}

void Logger::print_versions(const std::string& app_version, const std::string& libuv_version) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "ABOUT        " << RESET 
              << "duino-cpu/" << app_version << " gcc/clang\n";
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "LIBS         " << RESET 
              << libuv_version << "\n" << std::flush;
}

void Logger::print_cpu_info(const std::string& brand, int threads, const std::string& arch) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "CPU          " << RESET 
              << brand << "\n";
    std::cout << "                " << threads << " threads\n" << std::flush;
}

void Logger::print_pool_info(const std::string& pool, int port, const std::string& user) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "POOL         " << RESET 
              << pool << ":" << port << "\n";
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "USER         " << RESET 
              << user << "\n" << std::flush;
}

void Logger::print_commands() {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "COMMANDS     " << RESET 
              << "'h' hashrate, 'p' pause, 'r' resume, 'q' quit\n" 
              << std::flush;
}

void Logger::print_separator() {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << GRAY << "-------------------------------------------------------------------------------" 
              << RESET << "\n" << std::flush;
}

void Logger::benchmark_start(int threads) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_CPU << " "
              << WHITE << "starting benchmark with " << CYAN << threads 
              << WHITE << " threads" << RESET << "\n";
    std::cout << get_timestamp() << " " << TAG_CPU << " "
              << WHITE << "running for " << CYAN << "30 seconds" << RESET 
              << WHITE << "..." << RESET << "\n" << std::flush;
}

void Logger::benchmark_result(unsigned long total_hashes, double duration,
                             double total_hashrate, double per_thread) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    
    std::cout << get_timestamp() << " " << TAG_CPU << " "
              << BRIGHT_GREEN << "benchmark complete!" << RESET << "\n";
    
    std::cout << get_timestamp() << " " << TAG_CPU << " "
              << WHITE << "total hashes   " << CYAN 
              << total_hashes << RESET << "\n";
    
    std::cout << get_timestamp() << " " << TAG_CPU << " "
              << WHITE << "duration       " << CYAN 
              << std::fixed << std::setprecision(1) << duration 
              << WHITE << " seconds" << RESET << "\n";
    
    std::cout << get_timestamp() << " " << TAG_CPU << " "
              << WHITE << "hashrate       " << CYAN 
              << format_hashrate(total_hashrate) << RESET << "\n";
    
    std::cout << get_timestamp() << " " << TAG_CPU << " "
              << WHITE << "per thread     " << CYAN 
              << format_hashrate(per_thread) << RESET << "\n" << std::flush;
}

void Logger::cpu_summary(int total_threads, double total_hashrate) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_CPU << " " 
              << WHITE << "threads " << total_threads 
              << " hashrate " << CYAN << format_hashrate(total_hashrate) << RESET 
              << "\n" << std::flush;
}

void Logger::speed(const std::string& hashrate) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_MINER << " " << WHITE << hashrate << RESET << "\n" << std::flush;
}

void Logger::speed(int threads, double total_hashrate,
                  unsigned long accepted, unsigned long rejected) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << get_timestamp() << " " << TAG_MINER << " " 
              << WHITE << "threads " << threads 
              << " hashrate " << CYAN << format_hashrate(total_hashrate) << RESET
              << WHITE << " accepted " << accepted << " rejected " << rejected << RESET
              << "\n" << std::flush;
}

void Logger::print_stats(unsigned long accepted, unsigned long rejected,
                        double total_hashrate, int threads,
                        unsigned long uptime_seconds,
                        const std::string& pool_address, int pool_port,
                        unsigned long blocks) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    
    // Calculate stats
    double accept_rate = (accepted + rejected) > 0 ? 
        (accepted * 100.0 / (accepted + rejected)) : 100.0;
    
    // Format uptime
    auto format_uptime = [](unsigned long seconds) -> std::string {
        unsigned long days = seconds / 86400;
        seconds %= 86400;
        unsigned long hours = seconds / 3600;
        seconds %= 3600;
        unsigned long minutes = seconds / 60;
        seconds %= 60;
        
        std::stringstream ss;
        if (days > 0) {
            ss << days << "d " << hours << "h " << minutes << "m " << seconds << "s";
        } else if (hours > 0) {
            ss << hours << "h " << minutes << "m " << seconds << "s";
        } else if (minutes > 0) {
            ss << minutes << "m " << seconds << "s";
        } else {
            ss << seconds << "s";
        }
        return ss.str();
    };
    
    std::cout << "\n";
    std::cout << CYAN << "=========================================" << RESET << "\n\n";
    
    // Uptime
    std::cout << "  " << WHITE << BOLD << "Uptime:          " << RESET 
              << CYAN << format_uptime(uptime_seconds) << RESET << "\n";
    
    // Hashrate
    std::cout << "  " << WHITE << BOLD << "Hashrate:        " << RESET 
              << CHARTREUSE << format_hashrate(total_hashrate) << RESET 
              << GRAY << " (" << threads << " threads)" << RESET << "\n";
    
    // Shares
    std::cout << "  " << WHITE << BOLD << "Shares:          " << RESET;
    std::cout << CHARTREUSE << accepted << RESET << GRAY << " accepted" << RESET;
    std::cout << GRAY << " / " << RESET;
    std::cout << RED << rejected << RESET << GRAY << " rejected" << RESET;
    std::cout << GRAY << " (" << std::fixed << std::setprecision(1) 
              << accept_rate << "%)" << RESET << "\n";
    
    // Blocks
    if (blocks > 0) {
        std::cout << "  " << WHITE << BOLD << "Blocks Found:    " << RESET 
                  << YELLOW << BOLD << blocks << RESET << "\n";
    }
    
    // Pool
    std::cout << "  " << WHITE << BOLD << "Pool:            " << RESET 
              << CYAN << pool_address << ":" << pool_port << RESET << "\n";
    
    std::cout << "\n" << CYAN << "=========================================" << RESET << "\n\n";
    std::cout << std::flush;
}