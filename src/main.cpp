// Duino-Coin Official PC Miner 4.3 © MIT licensed
// https://duinocoin.com
// https://github.com/revoxhere/duino-coin
// Duino-Coin Team & Community 2019-2026
// Version 0.3beta 
#include "../include/config.h"
#include "../include/logger.h"
#include "../include/miner.h"
#include "../include/network.h"
#include "../include/benchmark.h"
#include <csignal>
#include <getopt.h>
#include <iostream>
#include <atomic>
#include <sys/prctl.h>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <sys/sysinfo.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <cpuid.h>
#endif

#define RESET         "\033[0m"
#define BOLD          "\033[1m"
#define CYAN          "\033[36m"
#define WHITE         "\033[37m"
#define GREEN         "\033[32m"
#define YELLOW        "\033[33m"
#define GRAY          "\033[90m"

std::atomic<bool> running(true);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = false;
        Logger::info("Shutdown signal received");
    }
}

std::string get_cpu_model() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                return line.substr(pos + 2);
            }
        }
    }
    return "Unknown CPU";
}

std::string get_memory_info() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        double total_gb = info.totalram / (1024.0 * 1024.0 * 1024.0);
        double free_gb = info.freeram / (1024.0 * 1024.0 * 1024.0);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << total_gb << " GB (" 
           << free_gb << " GB free)";
        return ss.str();
    }
    return "Unknown";
}

std::string get_cpu_avx_support() {
#if defined(__x86_64__) || defined(_M_X64)
    unsigned int eax, ebx, ecx, edx;
    
    // Check AVX-512
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        if (ebx & (1 << 16)) {  // AVX-512F
            return "AVX-512";
        }
    }
    
    // Check AVX2
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        if (ebx & (1 << 5)) {   // AVX2
            return "AVX2";
        }
    }
    
    // Check AVX
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        if (ecx & (1 << 28)) {  // AVX
            return "AVX";
        }
    }
    
    // Check SSE4.2
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        if (ecx & (1 << 20)) {  // SSE4.2
            return "SSE4.2";
        }
    }
    
    // Check SSE2
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        if (edx & (1 << 26)) {  // SSE2
            return "SSE2";
        }
    }
#endif
    return "none";
}

void print_banner() {
    std::cout << "\n";
    std::cout << CYAN << BOLD;
    std::cout << "  ____  _   _ ___ _   _  ___        ____ ____  _   _ \n";
    std::cout << " |  _ \\| | | |_ _| \\ | |/ _ \\      / ___|  _ \\| | | |\n";
    std::cout << " | | | | | | || ||  \\| | | | |____| |   | |_) | | | |\n";
    std::cout << " | |_| | |_| || || |\\  | |_| |____| |___|  __/| |_| |\n";
    std::cout << " |____/ \\___/|___|_| \\_|\\___/      \\____|_|    \\___/ \n";
    std::cout << RESET << "\n";
    std::cout << WHITE << " Official CPU Miner " << CYAN << "v" << VERSION << RESET << "\n";
    std::cout << GRAY << " https://duinocoin.com" << RESET << "\n";
    std::cout << GRAY << " https://github.com/duino-coin/duino-coin" << RESET << "\n\n";
}

void print_system_info(const Config& config) {
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "ABOUT        " << RESET 
              << "duino-cpu/" << VERSION << " gcc/clang\n";
    
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "LIBS         " << RESET;
    
#ifdef OPENSSL_VERSION_TEXT
    std::cout << "OpenSSL/" << OPENSSL_VERSION_TEXT;
#else
    std::cout << "OpenSSL/3.0+";
#endif
    
#ifdef LIBCURL_VERSION
    std::cout << " libcurl/" << LIBCURL_VERSION;
#else
    std::cout << " libcurl/8.0+";
#endif
    std::cout << "\n";
    
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "CPU          " << RESET 
              << get_cpu_model() << "\n";
    
    std::cout << "                " 
              << std::thread::hardware_concurrency() << " threads available\n";
    
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "MEMORY       " << RESET 
              << get_memory_info() << "\n";
    
    std::string avx = get_cpu_avx_support();
    std::cout << " " << CYAN << "* " << RESET 
              << WHITE << "SIMD         " << RESET;
    
    if (avx == "AVX-512") {
        std::cout << GREEN << "AVX-512 available" << RESET << "\n";
    } else if (avx == "AVX2") {
        std::cout << GREEN << "AVX2 available" << RESET << "\n";
    } else if (avx == "AVX") {
        std::cout << GREEN << "AVX available" << RESET << "\n";
    } else if (avx == "SSE4.2") {
        std::cout << GREEN << "SSE4.2 available" << RESET << "\n";
    } else if (avx == "SSE2") {
        std::cout << YELLOW << "SSE2 available" << RESET << "\n";
    } else {
        std::cout << YELLOW << "basic instruction set" << RESET << "\n";
    }

    std::cout << "\n";
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -u, --user <username>       Duino-Coin username (required)\n";
    std::cout << "  -k, --key <mining_key>      Mining key (optional)\n";
    std::cout << "  -t, --threads <number>      Number of threads (default: auto)\n";
    std::cout << "  -i, --intensity <1-100>     Mining intensity (default: 95)\n";
    std::cout << "  -d, --difficulty <type>     Starting difficulty: LOW, MEDIUM, NET (default: NET)\n";
    std::cout << "  -r, --rig <identifier>      Rig identifier (default: auto-generated)\n";
    std::cout << "  -p, --pool <host:port>      Custom pool address\n";
    std::cout << "  -b, --benchmark             Run benchmark and exit\n";
    std::cout << "  --invisible                 Hide process from htop/btop (stealth mode)\n";
    std::cout << "  --nolog                     Disable console logging\n";
    std::cout << "  -h, --help                  Show this help message\n\n";
}

void set_invisible_mode() {
    prctl(PR_SET_NAME, "[kworker/u8:0]", 0, 0, 0);
    int result __attribute__((unused)) = nice(19);
}

int main(int argc, char* argv[]) {
    Config config;
    bool benchmark_mode = false;
    bool show_help = false;

    static struct option long_options[] = {
        {"user", required_argument, 0, 'u'},
        {"key", required_argument, 0, 'k'},
        {"threads", required_argument, 0, 't'},
        {"intensity", required_argument, 0, 'i'},
        {"difficulty", required_argument, 0, 'd'},
        {"rig", required_argument, 0, 'r'},
        {"pool", required_argument, 0, 'p'},
        {"benchmark", no_argument, 0, 'b'},
        {"invisible", no_argument, 0, 'I'},
        {"nolog", no_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "u:k:t:i:d:r:p:bInh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'u': config.username = optarg; break;
            case 'k': config.mining_key = optarg; break;
            case 't': config.threads = std::stoi(optarg); break;
            case 'i': config.intensity = std::stoi(optarg); break;
            case 'd': config.start_diff = optarg; break;
            case 'r': config.rig_identifier = optarg; break;
            case 'p': {
                std::string pool_str = optarg;
                size_t colon_pos = pool_str.find(':');
                if (colon_pos != std::string::npos) {
                    config.pool_address = pool_str.substr(0, colon_pos);
                    config.pool_port = std::stoi(pool_str.substr(colon_pos + 1));
                } else {
                    Logger::error("Invalid pool format. Use: host:port");
                    return 1;
                }
                break;
            }
            case 'b': benchmark_mode = true; break;
            case 'I': config.invisible_mode = true; break;
            case 'n': Logger::disable(); break;
            case 'h': show_help = true; break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (config.invisible_mode) {
        set_invisible_mode();
    }

    print_banner();
    print_system_info(config);

    if (show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (config.username.empty() && !benchmark_mode) {
        Logger::error("Username is required. Use -u or --user");
        print_usage(argv[0]);
        return 1;
    }

    config.validate();

    if (benchmark_mode) {
        Benchmark bench;
        bench.run(config.threads);
        return 0;
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    NetworkManager network;
    if (!network.initialize()) {
        Logger::error("Failed to initialize network manager");
        return 1;
    }

    if (config.pool_address.empty()) {
        if (!network.fetch_pool()) {
            Logger::error("Failed to fetch mining pool");
            return 1;
        }
    } else {
        Logger::info("Using custom pool: " + config.pool_address + ":" + 
                     std::to_string(config.pool_port));
    }

    Miner miner(config, network);
    if (!miner.initialize()) {
        Logger::error("Failed to initialize miner");
        return 1;
    }

    miner.start();

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Logger::info("Stopping miner gracefully");
    miner.stop();
    Logger::success("Miner stopped successfully");
    return 0;
}