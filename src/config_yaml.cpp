#include "../include/config_yaml.h"
#include "../include/logger.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sys/stat.h>

bool ConfigYAML::load(const std::string& filename, Config& config) {
    try {
        YAML::Node yaml_config = YAML::LoadFile(filename);
        
        if (yaml_config["username"]) {
            config.username = yaml_config["username"].as<std::string>();
        }
        
        if (yaml_config["mining_key"]) {
            config.mining_key = yaml_config["mining_key"].as<std::string>();
        }
        
        if (yaml_config["rig_identifier"]) {
            config.rig_identifier = yaml_config["rig_identifier"].as<std::string>();
        }
        
        if (yaml_config["start_diff"]) {
            config.start_diff = yaml_config["start_diff"].as<std::string>();
        }
        
        if (yaml_config["pool"]) {
            YAML::Node pool = yaml_config["pool"];
            if (pool["address"]) {
                config.pool_address = pool["address"].as<std::string>();
            }
            if (pool["port"]) {
                config.pool_port = pool["port"].as<int>();
            }
        }
        
        if (yaml_config["threads"]) {
            config.threads = yaml_config["threads"].as<int>();
        }
        
        if (yaml_config["intensity"]) {
            config.intensity = yaml_config["intensity"].as<int>();
        }
        
        if (yaml_config["soc_timeout"]) {
            config.soc_timeout = yaml_config["soc_timeout"].as<int>();
        }
        
        if (yaml_config["report_interval"]) {
            config.report_interval = yaml_config["report_interval"].as<int>();
        }
        
        if (yaml_config["retry_delay"]) {
            config.retry_delay = yaml_config["retry_delay"].as<int>();
        }
        
        if (yaml_config["max_retries"]) {
            config.max_retries = yaml_config["max_retries"].as<int>();
        }
        
        if (yaml_config["invisible_mode"]) {
            config.invisible_mode = yaml_config["invisible_mode"].as<bool>();
        }
        
        Logger::success("Configuration loaded from " + filename);
        return true;
        
    } catch (const YAML::Exception& e) {
        Logger::error("Failed to load config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigYAML::save(const std::string& filename, const Config& config) {
    try {
        YAML::Emitter out;
        
        out << YAML::BeginMap;
        out << YAML::Comment("Duino-Coin CPU Miner Configuration");
        out << YAML::Newline;
        
        out << YAML::Key << "username" << YAML::Value << config.username;
        out << YAML::Key << "mining_key" << YAML::Value << config.mining_key;
        out << YAML::Key << "rig_identifier" << YAML::Value << config.rig_identifier;
        out << YAML::Newline;
        
        out << YAML::Key << "pool" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "address" << YAML::Value << config.pool_address;
        out << YAML::Key << "port" << YAML::Value << config.pool_port;
        out << YAML::EndMap;
        out << YAML::Newline;
        
        out << YAML::Key << "start_diff" << YAML::Value << config.start_diff;
        out << YAML::Key << "threads" << YAML::Value << config.threads;
        out << YAML::Key << "intensity" << YAML::Value << config.intensity;
        out << YAML::Newline;
        
        out << YAML::Key << "soc_timeout" << YAML::Value << config.soc_timeout;
        out << YAML::Key << "report_interval" << YAML::Value << config.report_interval;
        out << YAML::Key << "retry_delay" << YAML::Value << config.retry_delay;
        out << YAML::Key << "max_retries" << YAML::Value << config.max_retries;
        out << YAML::Newline;
        
        out << YAML::Key << "invisible_mode" << YAML::Value << config.invisible_mode;
        
        out << YAML::EndMap;
        
        std::ofstream fout(filename);
        fout << out.c_str();
        fout.close();
        
        Logger::success("Configuration saved to " + filename);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to save config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigYAML::create_default(const std::string& filename) {
    try {
        YAML::Emitter out;
        
        out << YAML::BeginMap;
        out << YAML::Comment("Duino-Coin CPU Miner Configuration");
        out << YAML::Comment("Edit this file to configure your miner");
        out << YAML::Newline;
        
        out << YAML::Key << "username" << YAML::Value << "your_username_here";
        out << YAML::Comment("Your Duino-Coin username");
        out << YAML::Newline;
        
        out << YAML::Key << "mining_key" << YAML::Value << "None";
        out << YAML::Comment("Mining key (optional)");
        out << YAML::Newline;
        
        out << YAML::Key << "rig_identifier" << YAML::Value << "Auto";
        out << YAML::Comment("Rig identifier (Auto = auto-generated)");
        out << YAML::Newline;
        
        out << YAML::Key << "pool" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "address" << YAML::Value << "";
        out << YAML::Comment("Leave empty for auto-selection");
        out << YAML::Key << "port" << YAML::Value << 0;
        out << YAML::EndMap;
        out << YAML::Newline;
        
        out << YAML::Key << "start_diff" << YAML::Value << "NET";
        out << YAML::Comment("Difficulty: LOW, MEDIUM, or NET");
        out << YAML::Newline;
        
        out << YAML::Key << "threads" << YAML::Value << 0;
        out << YAML::Comment("Number of threads (0 = auto)");
        out << YAML::Newline;
        
        out << YAML::Key << "intensity" << YAML::Value << 95;
        out << YAML::Comment("Mining intensity (1-100)");
        out << YAML::Newline;
        
        out << YAML::Key << "soc_timeout" << YAML::Value << 15;
        out << YAML::Key << "report_interval" << YAML::Value << 300;
        out << YAML::Key << "retry_delay" << YAML::Value << 5;
        out << YAML::Key << "max_retries" << YAML::Value << 3;
        out << YAML::Newline;
        
        out << YAML::Key << "invisible_mode" << YAML::Value << false;
        out << YAML::Comment("Hide process from htop/btop");
        
        out << YAML::EndMap;
        
        std::ofstream fout(filename);
        fout << out.c_str();
        fout.close();
        
        Logger::success("Default configuration created: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to create default config: " + std::string(e.what()));
        return false;
    }
}