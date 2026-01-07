#include "../include/miner.h"
#include "../include/hasher.h"
#include "../include/logger.h"
#include <chrono>
#include <sstream>
#include <thread>
#include <cstring>

Miner::Miner(const Config& cfg, NetworkManager& net) 
    : config(cfg), network(net) {}

Miner::~Miner() {
    stop();
}

bool Miner::initialize() {
    return true;
}

void Miner::start() {
    running = true;
    
    for (int i = 0; i < config.threads; i++) {
        threads.push_back(std::make_unique<std::thread>(
            &Miner::mining_thread, this, i));
    }
}

void Miner::stop() {
    running = false;
    
    for (auto& thread : threads) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }
    threads.clear();
}

bool Miner::get_job(SocketClient& client, std::string& last_hash, 
                   std::string& expected_hash, int& difficulty) {
    std::stringstream ss;
    ss << "JOB," << config.username << "," << config.start_diff << ","
       << config.mining_key;
    
    if (!client.send(ss.str())) {
        return false;
    }
    
    std::string response = client.receive(10);
    if (response.empty()) {
        return false;
    }
    
    std::istringstream iss(response);
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(iss, token, ',')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() < 3) {
        return false;
    }
    
    last_hash = tokens[0];
    expected_hash = tokens[1];
    difficulty = std::stoi(tokens[2]) * 100 + 1;
    
    return true;
}

bool Miner::submit_share(SocketClient& client, unsigned long result, 
                        double hashrate, int thread_id, int difficulty, 
                        double compute_time, int ping) {
    std::stringstream ss;
    ss << result << "," << std::fixed << std::setprecision(2) << hashrate 
       << ",Official CPU Miner " << VERSION 
       << "," << config.rig_identifier 
       << ",PC"  // Device type = PC
       << "," << config.miner_id;
    
    if (!client.send(ss.str())) {
        return false;
    }
    
    std::string response = client.receive(10);
    
    if (response.empty()) {
        return false;
    }
    
    // Remove any trailing whitespace/newline
    while (!response.empty() && (response.back() == '\n' || response.back() == '\r' || response.back() == ' ')) {
        response.pop_back();
    }
    
    std::istringstream iss(response);
    std::string status;
    std::getline(iss, status, ',');
    
    if (status == "GOOD") {
        stats.accepted++;
        Logger::share(thread_id, "ACCEPT", stats.accepted.load(), stats.rejected.load(),
                     hashrate, stats.total_hashrate.load(), compute_time, difficulty, ping);
        return true;
    } else if (status == "BLOCK") {
        stats.accepted++;
        stats.blocks++;
        Logger::share(thread_id, "BLOCK", stats.accepted.load(), stats.rejected.load(),
                     hashrate, stats.total_hashrate.load(), compute_time, difficulty, ping);
        return true;
    } else {
        stats.rejected++;
        Logger::share(thread_id, "REJECT", stats.accepted.load(), stats.rejected.load(),
                     hashrate, stats.total_hashrate.load(), compute_time, difficulty, ping);
        return false;
    }
}

void Miner::mining_thread(int thread_id) {
    SocketClient client;
    PoolInfo pool = network.get_pool();
    
    while (running) {
        if (!client.is_connected()) {
            if (!client.connect(pool.ip, pool.port, config.soc_timeout)) {
                Logger::error("Thread " + std::to_string(thread_id) + 
                            " connection failed, retrying...");
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            
            std::string version = client.receive(5);
            if (thread_id == 0) {
                Logger::info("Connected to pool version: " + version);
            }
        }
        
        std::string last_hash, expected_hash;
        int difficulty;
        
        // Measure ping time
        auto ping_start = std::chrono::high_resolution_clock::now();
        
        if (!get_job(client, last_hash, expected_hash, difficulty)) {
            Logger::warning("Thread " + std::to_string(thread_id) + 
                          " failed to get job");
            client.disconnect();
            continue;
        }
        
        auto ping_end = std::chrono::high_resolution_clock::now();
        int ping = std::chrono::duration_cast<std::chrono::milliseconds>(
            ping_end - ping_start).count();
        
        uint8_t expected_bytes[20];
        Hasher::hex_to_bytes(expected_hash, expected_bytes);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        bool found = false;
        for (unsigned long nonce = 0; nonce < (unsigned long)difficulty && running; nonce++) {
            std::string hash_input = last_hash + std::to_string(nonce);
            uint8_t hash_output[20];
            Hasher::ducos1_hash(hash_input, hash_output);
            
            if (memcmp(hash_output, expected_bytes, 20) == 0) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    end_time - start_time).count();
                
                double compute_time = duration / 1000000.0;
                double hashrate = duration > 0 ? 
                    (nonce * 1000000.0 / duration) : 0.0;
                
                stats.total_hashrate = hashrate;
                
                submit_share(client, nonce, hashrate, thread_id, difficulty, compute_time, ping);
                found = true;
                break;
            }
            
            if (config.intensity < 100 && nonce % 5000 == 0) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds((100 - config.intensity) * 10));
            }
        }
        
        if (!found && running) {
            // Timed out - disconnect and retry
            client.disconnect();
        }
    }
    
    client.disconnect();
}