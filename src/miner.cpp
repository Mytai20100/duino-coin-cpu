#include "../include/miner.h"
#include "../include/hasher.h"
#include "../include/logger.h"
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <cstring>
#include <algorithm>
#include <openssl/sha.h>

namespace OptimizedHasher {
    inline void fast_uint_to_str(unsigned long num, char* buffer, int& len) {
        if (num == 0) {
            buffer[0] = '0';
            len = 1;
            return;
        }
        
        char temp[32];
        int i = 0;
        while (num > 0) {
            temp[i++] = '0' + (num % 10);
            num /= 10;
        }
        
        len = i;
        for (int j = 0; j < i; j++) {
            buffer[j] = temp[i - 1 - j];
        }
    }
    
    inline void hash_with_nonce(const char* base, size_t base_len,
                               unsigned long nonce, uint8_t output[20]) {
        char buffer[256];
        memcpy(buffer, base, base_len);
        
        int nonce_len;
        fast_uint_to_str(nonce, buffer + base_len, nonce_len);
        
        ::SHA1((unsigned char*)buffer, base_len + nonce_len, output);
    }
}

Miner::Miner(const Config& cfg, NetworkManager& net) 
    : config(cfg), network(net) {
    stats.thread_hashrates.resize(cfg.threads, 0.0);
}

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
#ifdef __linux__
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i % std::thread::hardware_concurrency(), &cpuset);
        pthread_setaffinity_np(threads.back()->native_handle(), 
                              sizeof(cpu_set_t), &cpuset);
#endif
    }
    
    threads.push_back(std::make_unique<std::thread>([this]() {
        auto last_update = std::chrono::steady_clock::now();
        
        while (running) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - last_update).count();
            
            if (elapsed >= 10) {
                double total_hr = 0.0;
                {
                    std::lock_guard<std::mutex> lock(stats.hashrate_mutex);
                    for (double hr : stats.thread_hashrates) {
                        total_hr += hr;
                    }
                }
                
                Logger::speed_update(
                    config.threads,
                    total_hr,
                    stats.accepted.load(),
                    stats.rejected.load()
                );
                last_update = now;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << std::endl;
    }));
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
    static thread_local char send_buffer[256];
    int len = snprintf(send_buffer, sizeof(send_buffer), 
                      "JOB,%s,%s,%s",
                      config.username.c_str(),
                      config.start_diff.c_str(),
                      config.mining_key.c_str());
    
    if (!client.send(std::string(send_buffer, len))) {
        return false;
    }
    
    std::string response = client.receive(10);
    if (response.empty()) {
        return false;
    }
    
    const char* ptr = response.c_str();
    const char* comma1 = strchr(ptr, ',');
    if (!comma1) return false;
    
    const char* comma2 = strchr(comma1 + 1, ',');
    if (!comma2) return false;
    
    last_hash.assign(ptr, comma1 - ptr);
    expected_hash.assign(comma1 + 1, comma2 - comma1 - 1);
    difficulty = atoi(comma2 + 1) * 100 + 1;
    
    return true;
}

bool Miner::submit_share(SocketClient& client, unsigned long result, 
                        double hashrate, int thread_id, int difficulty, 
                        double compute_time, int ping) {
    static thread_local char send_buffer[512];
    // CHỈ SỬA DÒNG NÀY - Đổi "PC" thành "" để có 2 dấu phẩy liên tiếp
    int len = snprintf(send_buffer, sizeof(send_buffer),
                      "%lu,%.2f,Official PC Miner %s,%s,,%s",
                      result, hashrate, VERSION,
                      config.rig_identifier.c_str(),
                      config.miner_id.c_str());
    
    if (!client.send(std::string(send_buffer, len))) {
        return false;
    }
    
    std::string response = client.receive(10);
    if (response.empty()) {
        return false;
    }
    
    while (!response.empty() && 
           (response.back() == '\n' || response.back() == '\r' || 
            response.back() == ' ')) {
        response.pop_back();
    }
    
    bool is_good = (response.compare(0, 4, "GOOD") == 0);
    bool is_block = (response.compare(0, 5, "BLOCK") == 0);
    
    if (is_good || is_block) {
        stats.accepted++;
        if (is_block) {
            stats.blocks++;
        }
        
        double total_hr = 0.0;
        {
            std::lock_guard<std::mutex> lock(stats.hashrate_mutex);
            for (double hr : stats.thread_hashrates) {
                total_hr += hr;
            }
        }
        
        Logger::share(thread_id, is_block ? "BLOCK" : "ACCEPT", 
                     stats.accepted.load(), stats.rejected.load(), 
                     hashrate, total_hr, 
                     compute_time, difficulty, ping);
        return true;
    } else {
        stats.rejected++;
        
        double total_hr = 0.0;
        {
            std::lock_guard<std::mutex> lock(stats.hashrate_mutex);
            for (double hr : stats.thread_hashrates) {
                total_hr += hr;
            }
        }
        
        Logger::share(thread_id, "REJECT", stats.accepted.load(), 
                     stats.rejected.load(), hashrate, total_hr, 
                     compute_time, difficulty, ping);
        return false;
    }
}

void Miner::mining_thread(int thread_id) {
    SocketClient client;
    PoolInfo pool = network.get_pool();
    static thread_local uint8_t expected_bytes[20];
    static thread_local uint8_t hash_output[20];
    
    while (running) {
        if (!client.is_connected()) {
            if (thread_id == 0) {
                Logger::net_connect(pool.ip, pool.port);
            }
            
            auto connect_start = std::chrono::high_resolution_clock::now();
            
            if (!client.connect(pool.ip, pool.port, config.soc_timeout)) {
                if (thread_id == 0) {
                    Logger::net_error("Connection failed");
                }
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            
            std::string version = client.receive(5);
            
            auto connect_end = std::chrono::high_resolution_clock::now();
            int connect_ping = std::chrono::duration_cast<std::chrono::milliseconds>(
                connect_end - connect_start).count();
            
            if (thread_id == 0) {
                Logger::net_connected(version, connect_ping);
            }
        }
        
        std::string last_hash, expected_hash;
        int difficulty;
        
        auto ping_start = std::chrono::high_resolution_clock::now();
        
        if (!get_job(client, last_hash, expected_hash, difficulty)) {
            client.disconnect();
            continue;
        }
        
        auto ping_end = std::chrono::high_resolution_clock::now();
        int ping = std::chrono::duration_cast<std::chrono::milliseconds>(
            ping_end - ping_start).count();
        
        Hasher::hex_to_bytes(expected_hash, expected_bytes);
        
        const char* last_hash_cstr = last_hash.c_str();
        size_t last_hash_len = last_hash.length();
        
        auto start_time = std::chrono::high_resolution_clock::now();
        unsigned long hashes_done = 0;
        
        bool found = false;
        unsigned long difficulty_ul = (unsigned long)difficulty;
        
        for (unsigned long nonce = 0; nonce < difficulty_ul && running; nonce++) {
            if (nonce + 16 < difficulty_ul) {
                __builtin_prefetch((void*)(nonce + 16));
            }
            
            OptimizedHasher::hash_with_nonce(last_hash_cstr, last_hash_len, 
                                            nonce, hash_output);
            hashes_done++;
            
#if defined(USE_AVX2)
            bool match = Hasher::ducos1_compare_avx2(hash_output, expected_bytes);
#elif defined(USE_AVX512)
            bool match = Hasher::ducos1_compare_avx512(hash_output, expected_bytes);
#else
            bool match = (memcmp(hash_output, expected_bytes, 20) == 0);
#endif
            
            if (match) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    end_time - start_time).count();
                
                double compute_time = duration / 1000000.0;
                double hashrate = duration > 0 ? 
                    (hashes_done * 1000000.0 / duration) : 0.0;
                
                {
                    std::lock_guard<std::mutex> lock(stats.hashrate_mutex);
                    stats.thread_hashrates[thread_id] = hashrate;
                }
                
                submit_share(client, nonce, hashrate, thread_id, 
                           difficulty, compute_time, ping);
                found = true;
                break;
            }
            
            if ((hashes_done & 0xFFFF) == 0) {
                auto current_time = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                    current_time - start_time).count();
                
                if (elapsed > 0) {
                    double current_hashrate = hashes_done * 1000000.0 / elapsed;
                    std::lock_guard<std::mutex> lock(stats.hashrate_mutex);
                    stats.thread_hashrates[thread_id] = current_hashrate;
                }
                
                if (config.intensity < 100) {
                    std::this_thread::sleep_for(
                        std::chrono::microseconds((100 - config.intensity) * 10));
                }
            }
        }
        
        if (!found && running) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time).count();
            
            if (duration > 0) {
                double final_hashrate = hashes_done * 1000000.0 / duration;
                std::lock_guard<std::mutex> lock(stats.hashrate_mutex);
                stats.thread_hashrates[thread_id] = final_hashrate;
            }
            
            client.disconnect();
        }
    }
    
    client.disconnect();
}