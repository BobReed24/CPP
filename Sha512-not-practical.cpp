#include <iostream>
#include <fstream>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstring>

std::string sha512(const std::string& data) {
    unsigned char hash[SHA512_DIGEST_LENGTH];
    SHA512((const unsigned char*)data.c_str(), data.length(), hash);

    std::ostringstream os;
    for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i)
        os << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    return os.str();
}

int main() {
    uint64_t count = 0;
    std::ofstream outfile("output.txt", std::ios::out | std::ios::app);
    if (!outfile) {
        std::cerr << "Failed to open output.txt for writing\n";
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    while (true) {
        std::string input = std::to_string(count);
        std::string hash = sha512(input);
        outfile << hash << '\n';

        if (++count % 1000000 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(now - start).count();
            std::cout << "Hashes: " << count << " | Time: " << elapsed << "s\n";
            outfile.flush(); // optional safety
        }
    }

    return 0;
}
