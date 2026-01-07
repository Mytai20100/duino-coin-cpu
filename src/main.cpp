
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

std::atomic<bool> running(true);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = false;
        Logger::info("Shutdown signal received");
    }
}

void print_banner() {
    std::cout << "\n";
    std::cout << "  ____  _   _ ___ _   _  ___        ____ ____  _   _ \n";
    std::cout << " |  _ \\| | | |_ _| \\ | |/ _ \\      / ___|  _ \\| | | |\n";
    std::cout << " | | | | | | || ||  \\| | | | |____| |   | |_) | | | |\n";
    std::cout << " | |_| | |_| || || |\\  | |_| |____| |___|  __/| |_| |\n";
    std::cout << " |____/ \\___/|___|_| \\_|\\___/      \\____|_|    \\___/ \n";
    std::cout << "\n";
    std::cout << " Official CPU Miner v" << VERSION << "\n";
    std::cout << " https://duinocoin.com\n";
    std::cout << " https://github.com/duino-coin/duino-coin\n\n";
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
    std::cout << "  -b, --benchmark             Run benchmark and exit\n";
    std::cout << "  -inv, --invisible           Hide process from htop/btop (stealth mode)\n";
    std::cout << "  --nolog                     Disable console logging\n";
    std::cout << "  -h, --help                  Show this help message\n";
    std::cout << "\n";
}

void set_invisible_mode() {
    // Change process name to look like system process
    prctl(PR_SET_NAME, "[kworker/u8:0]", 0, 0, 0);
    
    // Try to lower priority to be less visible
    nice(19);
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
        {"benchmark", no_argument, 0, 'b'},
        {"invisible", no_argument, 0, 'v'},
        {"nolog", no_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "u:k:t:i:d:r:bnvh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'u':
                config.username = optarg;
                break;
            case 'k':
                config.mining_key = optarg;
                break;
            case 't':
                config.threads = std::stoi(optarg);
                break;
            case 'i':
                config.intensity = std::stoi(optarg);
                break;
            case 'd':
                config.start_diff = optarg;
                break;
            case 'r':
                config.rig_identifier = optarg;
                break;
            case 'b':
                benchmark_mode = true;
                break;
            case 'v':
                config.invisible_mode = true;
                break;
            case 'n':
                Logger::disable();
                break;
            case 'h':
                show_help = true;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (config.invisible_mode) {
        set_invisible_mode();
    }

    print_banner();

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

    config.print();

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    NetworkManager network;
    if (!network.initialize()) {
        Logger::error("Failed to initialize network manager");
        return 1;
    }

    if (!network.fetch_pool()) {
        Logger::error("Failed to fetch mining pool");
        return 1;
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

    auto stats = miner.get_stats();
    Logger::info("Final statistics:");
    Logger::info("  Accepted shares: " + std::to_string(stats.accepted));
    Logger::info("  Rejected shares: " + std::to_string(stats.rejected));
    Logger::info("  Blocks found: " + std::to_string(stats.blocks));
    
    if (stats.accepted + stats.rejected > 0) {
        double accept_rate = (stats.accepted * 100.0) / (stats.accepted + stats.rejected);
        Logger::info("  Accept rate: " + std::to_string(accept_rate) + "%");
    }
    Logger::success("Miner stopped successfully");
    
    return 0;
}