#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <atomic>
#include <chrono>

const int THREAD_COUNT = std::thread::hardware_concurrency();
constexpr int BATCH_SIZE = 10000;
constexpr int FLUSH_THRESHOLD = 100000;

std::mutex file_mutex;
std::atomic<uint64_t> global_counter(0);
std::atomic<uint64_t> total_hashed(0);

std::string sha512(const std::string &data) {
    unsigned char hash[SHA512_DIGEST_LENGTH];
    SHA512(reinterpret_cast<const unsigned char *>(data.c_str()), data.length(), hash);

    std::ostringstream os;
    for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i)
        os << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return os.str();
}

void worker(std::ofstream &outfile, std::mutex &out_mutex) {
    std::vector<std::string> local_buffer;
    local_buffer.reserve(BATCH_SIZE);

    while (true) {
        uint64_t base = global_counter.fetch_add(BATCH_SIZE);
        for (int i = 0; i < BATCH_SIZE; ++i) {
            std::string input = std::to_string(base + i);
            std::string hash = sha512(input);
            local_buffer.push_back(hash);
        }

        {
            std::lock_guard<std::mutex> lock(out_mutex);
            for (const auto &h : local_buffer)
                outfile << h << '\n';
        }

        total_hashed += BATCH_SIZE;
        local_buffer.clear();
    }
}

int main() {
    std::ofstream outfile("output.txt", std::ios::out | std::ios::app);
    if (!outfile) {
        std::cerr << "Failed to open output.txt for writing\n";
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back(worker, std::ref(outfile), std::ref(file_mutex));
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        std::cout << "[" << elapsed << "s] Hashes computed: " << total_hashed.load() << "\n";
    }

    for (auto &t : threads) t.join();
    return 0;
}
