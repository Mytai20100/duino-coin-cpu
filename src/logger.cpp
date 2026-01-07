#include "../include/logger.h"
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

static bool enabled = true;
static std::mutex log_mutex;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define GRAY    "\033[90m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"

static std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time), "%d/%m/%Y %H:%M:%S") << "]";
    return ss.str();
}

std::string Logger::format_hashrate(double hashrate) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
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
    
    if (difficulty >= 1000) {
        ss << std::fixed << std::setprecision(1);
        ss << (difficulty / 1000.0) << "k";
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
    std::cout << DIM << get_timestamp() << " " << RESET 
              << BLUE << "INFO  " << RESET << message << "\n" << std::flush;
}

void Logger::success(const std::string& message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << DIM << get_timestamp() << " " << RESET 
              << GREEN << "OK    " << RESET << message << "\n" << std::flush;
}

void Logger::warning(const std::string& message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << DIM << get_timestamp() << " " << RESET 
              << YELLOW << "WARN  " << RESET << message << "\n" << std::flush;
}

void Logger::error(const std::string& message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << DIM << get_timestamp() << " " << RESET 
              << RED << "ERROR " << RESET << message << "\n" << std::flush;
}

void Logger::share(int thread_id, const std::string& status, 
                  unsigned long accepted, unsigned long rejected,
                  double hashrate, double total_hashrate,
                  double compute_time, int difficulty, int ping) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    
    std::string color = GREEN;
    if (status == "REJECT") color = RED;
    else if (status == "BLOCK") color = YELLOW;
    
    double accept_rate = (accepted + rejected) > 0 ? 
        (accepted * 100.0 / (accepted + rejected)) : 0.0;
    
    std::cout << DIM << get_timestamp() << " " << RESET 
              << CYAN << "cpu" << thread_id << " " << RESET
              << color << status << " " << RESET
              << accepted << "/" << (accepted + rejected) 
              << " (" << std::fixed << std::setprecision(1) << accept_rate << "%) "
              << DIM << compute_time << "s " << RESET
              << BOLD << format_hashrate(hashrate) << " " << RESET
              << DIM << "(" << format_hashrate(total_hashrate) << " total) " << RESET
              << "diff " << format_difficulty(difficulty) 
              << " ping " << ping << "ms\n" << std::flush;
}