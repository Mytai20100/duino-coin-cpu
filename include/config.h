#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <iostream>
#include <thread>
#include <random>
#include <sstream>
#include <iomanip>

#define VERSION "0.0.1"
#define SEPARATOR ","

class Config {
public:
    std::string username;
    std::string mining_key = "None";
    std::string rig_identifier = "Auto";
    std::string start_diff = "NET";
    int threads = 0;
    int intensity = 95;
    int soc_timeout = 10;
    int report_interval = 300;
    std::string miner_id;
    bool invisible_mode = false;  // New flag for -inv

    Config() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 2811);
        miner_id = std::to_string(dis(gen));
    }

    void validate() {
        if (threads <= 0) {
            threads = std::thread::hardware_concurrency();
        }
        if (threads > 128) {
            threads = 128;
        }
        if (intensity < 1) intensity = 1;
        if (intensity > 100) intensity = 100;

        if (start_diff != "LOW" && start_diff != "MEDIUM" && start_diff != "NET") {
            start_diff = "NET";
        }

        if (rig_identifier == "Auto") {
            rig_identifier = generate_rig_id();
        }
    }

    void print() const {
        std::cout << "Configuration:\n";
        std::cout << "  Username: " << username << "\n";
        std::cout << "  Mining Key: " << (mining_key != "None" ? "Set" : "None") << "\n";
        std::cout << "  Rig ID: " << rig_identifier << "\n";
        std::cout << "  Threads: " << threads << "\n";
        std::cout << "  Difficulty: " << start_diff << "\n";
        std::cout << "  Intensity: " << intensity << "%\n";
        if (invisible_mode) {
            std::cout << "  Invisible Mode: Enabled\n";
        }
        std::cout << "\n";
    }

private:
    std::string generate_rig_id() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 0xFFFFFF);
        std::stringstream ss;
        ss << "CPU-" << std::hex << std::uppercase << std::setfill('0') << std::setw(6) << dis(gen);
        return ss.str();
    }
};

#endif