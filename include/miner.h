#ifndef MINER_H
#define MINER_H

#include "config.h"
#include "network.h"
#include <atomic>
#include <vector>
#include <thread>
#include <memory>
#include <mutex>

struct MiningStats {
    std::atomic<unsigned long> accepted{0};
    std::atomic<unsigned long> rejected{0};
    std::atomic<unsigned long> blocks{0};
    std::vector<double> thread_hashrates;
    mutable std::mutex hashrate_mutex;
};

struct MiningStatsSnapshot {
    unsigned long accepted;
    unsigned long rejected;
    unsigned long blocks;
    double total_hashrate;
};

class Miner {
private:
    Config config;
    NetworkManager& network;
    MiningStats stats;
    std::vector<std::unique_ptr<std::thread>> threads;
    std::atomic<bool> running{false};
    
    void mining_thread(int thread_id);
    bool get_job(SocketClient& client, std::string& last_hash, 
                 std::string& expected_hash, int& difficulty);
    bool submit_share(SocketClient& client, unsigned long result, 
                     double hashrate, int thread_id, int difficulty,
                     double compute_time, int ping);
    
public:
    Miner(const Config& cfg, NetworkManager& net);
    ~Miner();
    
    bool initialize();
    void start();
    void stop();
    MiningStatsSnapshot get_stats() const {
        std::lock_guard<std::mutex> lock(stats.hashrate_mutex);
        double total = 0.0;
        for (double hr : stats.thread_hashrates) {
            total += hr;
        }
        return {
            stats.accepted.load(),
            stats.rejected.load(),
            stats.blocks.load(),
            total
        };
    }
};

#endif