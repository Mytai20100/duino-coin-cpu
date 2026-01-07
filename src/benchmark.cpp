#include "../include/benchmark.h"
#include "../include/hasher.h"
#include "../include/logger.h"
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <iomanip>

static std::atomic<unsigned long> total_hashes{0};

void benchmark_worker(int duration_seconds) {
    auto end_time = std::chrono::steady_clock::now() + 
                    std::chrono::seconds(duration_seconds);
    
    unsigned long local_hashes = 0;
    std::string test_data = "duinocoin_benchmark_test";
    uint8_t output[20];
    
    while (std::chrono::steady_clock::now() < end_time) {
        for (int i = 0; i < 1000; i++) {
            Hasher::ducos1_hash(test_data + std::to_string(local_hashes + i), output);
        }
        local_hashes += 1000;
    }
    
    total_hashes += local_hashes;
}

void Benchmark::run(int threads) {
    Logger::info("Starting benchmark with " + std::to_string(threads) + " threads");
    Logger::info("Running for 30 seconds...");
    
    total_hashes = 0;
    std::vector<std::thread> workers;
    
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < threads; i++) {
        workers.emplace_back(benchmark_worker, 30);
    }
    
    for (auto& worker : workers) {
        worker.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count() / 1000.0;
    
    double hashrate = total_hashes / duration;
    
    Logger::success("Benchmark complete!");
    Logger::info("Total hashes: " + std::to_string(total_hashes.load()));
    Logger::info("Duration: " + std::to_string(duration) + " seconds");
    Logger::info("Hashrate: " + std::to_string(hashrate/1000.0) + " kH/s");
    Logger::info("Per thread: " + std::to_string(hashrate/threads/1000.0) + " kH/s");
}