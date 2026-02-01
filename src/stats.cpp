#include "../include/stats.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <vector>

std::string SystemStats::get_uptime() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
    long seconds = duration.count();
    
    long days = seconds / 86400;
    seconds %= 86400;
    long hours = seconds / 3600;
    seconds %= 3600;
    long minutes = seconds / 60;
    
    std::stringstream ss;
    if (days > 0) {
        ss << days << "d " << hours << "h " << minutes << "m";
    } else if (hours > 0) {
        ss << hours << "h " << minutes << "m";
    } else {
        ss << minutes << "m";
    }
    
    return ss.str();
}

double SystemStats::get_cpu_usage() const {
    static unsigned long long last_total = 0;
    static unsigned long long last_idle = 0;
    
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        return 0.0;
    }
    
    std::string line;
    std::getline(stat_file, line);
    stat_file.close();
    
    std::istringstream ss(line);
    std::string cpu;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long total_idle = idle + iowait;
    
    if (last_total == 0) {
        last_total = total;
        last_idle = total_idle;
        usleep(100000); // 100ms
        return get_cpu_usage();
    }
    
    unsigned long long total_diff = total - last_total;
    unsigned long long idle_diff = total_idle - last_idle;
    
    last_total = total;
    last_idle = total_idle;
    
    if (total_diff == 0) {
        return 0.0;
    }
    
    return 100.0 * (1.0 - static_cast<double>(idle_diff) / static_cast<double>(total_diff));
}

std::string SystemStats::get_cpu_name() const {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                std::string name = line.substr(pos + 2);
                // Trim whitespace
                size_t start = name.find_first_not_of(" \t");
                size_t end = name.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    return name.substr(start, end - start + 1);
                }
            }
        }
    }
    
    return "Unknown CPU";
}

std::string StatsDisplay::format_bytes(unsigned long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 3) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return ss.str();
}

std::string StatsDisplay::format_uptime(long seconds) {
    long days = seconds / 86400;
    seconds %= 86400;
    long hours = seconds / 3600;
    seconds %= 3600;
    long minutes = seconds / 60;
    
    std::stringstream ss;
    if (days > 0) {
        ss << days << "d " << hours << "h " << minutes << "m";
    } else if (hours > 0) {
        ss << hours << "h " << minutes << "m";
    } else {
        ss << minutes << "m";
    }
    
    return ss.str();
}

void StatsDisplay::show_stats(const SystemStats& sys_stats,
                              unsigned long accepted,
                              unsigned long rejected,
                              const std::string& pool_address,
                              int pool_port) {
    std::cout << "\n";
    std::cout << "================================ STATS ================================\n";
    
    // Uptime
    std::cout << "Uptime:        " << sys_stats.get_uptime() << "\n";
    
    // CPU info
    std::string cpu_name = sys_stats.get_cpu_name();
    double cpu_usage = sys_stats.get_cpu_usage();
    std::cout << "CPU:           " << cpu_name 
              << " (" << std::fixed << std::setprecision(0) << cpu_usage << "%)\n";
    
    // Shares
    std::cout << "Shares:        Accepted: " << accepted 
              << " | Rejected: " << rejected;
    if (accepted + rejected > 0) {
        double accept_rate = (accepted * 100.0) / (accepted + rejected);
        std::cout << " (" << std::fixed << std::setprecision(1) << accept_rate << "%)";
    }
    std::cout << "\n";
    
    // Data usage
    unsigned long total_data = sys_stats.data_bytes_sent.load() + sys_stats.data_bytes_received.load();
    std::cout << "Data Used:     " << format_bytes(total_data) 
              << " (Sent: " << format_bytes(sys_stats.data_bytes_sent.load())
              << " | Received: " << format_bytes(sys_stats.data_bytes_received.load()) << ")\n";
    
    // Pool info
    std::cout << "Pool:          " << pool_address << ":" << pool_port << "\n";
    
    std::cout << "=======================================================================\n";
    std::cout << "\n";
}