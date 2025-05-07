// Command to compile: g++ Pi_calculation.cpp -o Pi_calculation -lmpfr -lgmp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <limits>
#include <sstream>
#include <iomanip>
#include <algorithm> // For std::min
#include <utility>   // For std::pair

// Include GMP and MPFR headers
#include <gmp.h>
#include <mpfr.h>

// --- System Usage Implementation (Linux-specific approximation of psutil) ---
// Note: This is a simplified implementation for Linux using /proc filesystem.
// It does not replicate psutil's cross-platform capabilities exactly.
// CPU usage is calculated based on the difference in /proc/stat readings
// between calls, approximating psutil's interval=0 behavior after the first call.
// RAM usage is calculated based on MemTotal and MemAvailable from /proc/meminfo.

struct SystemStats {
    long long cpu_user = 0;
    long long cpu_nice = 0;
    long long cpu_system = 0;
    long long cpu_idle = 0;
    long long cpu_iowait = 0;
    long long cpu_irq = 0;
    long long cpu_softirq = 0;
    long long cpu_steal = 0;
    long long cpu_guest = 0;
    long long cpu_guest_nice = 0;
    long long mem_total = 0;
    long long mem_available = 0;
};

// Function to read current system stats from /proc
SystemStats read_system_stats() {
    SystemStats stats;
    std::ifstream stat_file("/proc/stat");
    if (stat_file.is_open()) {
        std::string line;
        std::getline(stat_file, line);
        stat_file.close();
        // Parse cpu line
        std::stringstream ss(line);
        std::string cpu_label;
        ss >> cpu_label >> stats.cpu_user >> stats.cpu_nice >> stats.cpu_system >> stats.cpu_idle >> stats.cpu_iowait >> stats.cpu_irq >> stats.cpu_softirq >> stats.cpu_steal >> stats.cpu_guest >> stats.cpu_guest_nice;
    } else {
        std::cerr << "Warning: Could not open /proc/stat. CPU usage will be 0." << std::endl;
    }

    std::ifstream meminfo_file("/proc/meminfo");
    if (meminfo_file.is_open()) {
        std::string line;
        while (std::getline(meminfo_file, line)) {
            if (line.rfind("MemTotal:", 0) == 0) { // Starts with "MemTotal:"
                std::stringstream ss(line);
                std::string label;
                ss >> label >> stats.mem_total; // Value is in KB
            } else if (line.rfind("MemAvailable:", 0) == 0) { // Starts with "MemAvailable:"
                 std::stringstream ss(line);
                std::string label;
                ss >> label >> stats.mem_available; // Value is in KB
            }
        }
        meminfo_file.close();
    } else {
         std::cerr << "Warning: Could not open /proc/meminfo. RAM usage will be 0." << std::endl;
    }
    return stats;
}

// Global static variable to store previous CPU stats for calculating usage percentage
static SystemStats prev_stats;
static bool first_cpu_call = true;

// Function to get CPU usage percentage (since last call)
double get_cpu_usage_percent(const SystemStats& current_stats) {
    if (first_cpu_call) {
        prev_stats = current_stats;
        first_cpu_call = false;
        // On the first call, calculate usage since boot or return 0.
        // psutil interval=0 on first call often shows usage since boot.
        long long total_time = current_stats.cpu_user + current_stats.cpu_nice + current_stats.cpu_system + current_stats.cpu_idle + current_stats.cpu_iowait + current_stats.cpu_irq + current_stats.cpu_softirq + current_stats.cpu_steal + current_stats.cpu_guest + current_stats.cpu_guest_nice;
        long long idle_time = current_stats.cpu_idle + current_stats.cpu_iowait;
        if (total_time == 0) return 0.0;
        return 100.0 * (total_time - idle_time) / total_time;
    } else {
        // Calculate difference since last call
        long long prev_total_time = prev_stats.cpu_user + prev_stats.cpu_nice + prev_stats.cpu_system + prev_stats.cpu_idle + prev_stats.cpu_iowait + prev_stats.cpu_irq + prev_stats.cpu_softirq + prev_stats.cpu_steal + prev_stats.cpu_guest + prev_stats.cpu_guest_nice;
        long long prev_idle_time = prev_stats.cpu_idle + prev_stats.cpu_iowait;

        long long current_total_time = current_stats.cpu_user + current_stats.cpu_nice + current_stats.cpu_system + current_stats.cpu_idle + current_stats.cpu_iowait + current_stats.cpu_irq + current_stats.cpu_softirq + current_stats.cpu_steal + current_stats.cpu_guest + current_stats.cpu_guest_nice;
        long long current_idle_time = current_stats.cpu_idle + current_stats.cpu_iowait;

        long long total_diff = current_total_time - prev_total_time;
        long long idle_diff = current_idle_time - prev_idle_time;

        // Update previous stats for the next call
        prev_stats = current_stats;

        if (total_diff <= 0) return 0.0; // Avoid division by zero or negative diff
        return 100.0 * (total_diff - idle_diff) / total_diff;
    }
}

// Function to get RAM usage percentage
double get_ram_usage_percent(const SystemStats& stats) {
    if (stats.mem_total == 0) return 0.0;
    // psutil uses virtual_memory().percent which is (total - available) / total * 100
    return 100.0 * (stats.mem_total - stats.mem_available) / stats.mem_total;
}


// Returns the current CPU and RAM usage percentages.
// Note: This is a simplified, Linux-specific implementation.
// CPU usage is calculated since the last call of this function.
// RAM usage is calculated based on MemTotal and MemAvailable.
std::pair<double, double> get_system_usage() {
    SystemStats current_stats = read_system_stats();
    double cpu_usage = get_cpu_usage_percent(current_stats);
    double ram_usage = get_ram_usage_percent(current_stats);
    return {cpu_usage, ram_usage};
}

// --- Pi Calculation using MPFR ---

// Incrementally calculate pi to a specified number of digits and write to a file.
//
// :param file_path: Path to the file where the result will be stored.
// :param digits: Total number of digits to calculate. Use std::numeric_limits<double>::infinity() for infinite calculation.
// :param chunk_size: Number of digits to calculate per step.
void calculate_pi_to_file(const std::string& file_path, double digits, long long chunk_size = 100000) { // Default chunk size is 100,000

    // mp.dps equivalent in MPFR is setting precision in bits.
    // Precision in bits = ceil(dps * log2(10))
    // We will set precision dynamically inside the loop as in the Python code.

    auto start_time = std::chrono::high_resolution_clock::now();

    // Open file in write mode to clear it and write "3."
    std::ofstream file_w(file_path, std::ios::out | std::ios::trunc);
    if (!file_w.is_open()) {
        std::cerr << "Error opening file for writing: " << file_path << std::endl;
        return;
    }
    file_w << "3.";
    file_w.close(); // Close the write stream

    std::cout << "Added \"3.\" to the beginning of the file." << std::endl;

    // Open file in append mode for writing digits
    // Use std::ios::binary to potentially improve performance for large writes,
    // although digits are text. std::ios::app implies std::ios::out.
    std::ofstream file_a(file_path, std::ios::app | std::ios::binary);
     if (!file_a.is_open()) {
        std::cerr << "Error opening file for appending: " << file_path << std::endl;
        return;
    }

    long long total_written = 0;

    // Check if digits is infinity
    bool is_infinite = std::isinf(digits);
    long long total_digits_ll = is_infinite ? -1 : static_cast<long long>(digits); // Use -1 to signify infinite in print

    while (is_infinite || total_written < total_digits_ll) {

        long long remaining_digits;
        if (is_infinite) {
            remaining_digits = chunk_size;
        } else {
            remaining_digits = std::min(chunk_size, total_digits_ll - total_written);
        }

        if (remaining_digits <= 0) {
             if (!is_infinite) break; // Stop if finite digits are reached and remaining is 0 or less
             // If infinite, remaining_digits should always be chunk_size > 0, loop continues
        }

        // Calculate required precision in bits for total_written + remaining_digits decimal places
        // Need total_written + remaining_digits digits after decimal.
        long long required_dps = total_written + remaining_digits;
        if (required_dps <= 0) required_dps = 1; // MPFR precision must be >= 1 decimal place

        // Precision in bits = ceil(dps * log2(10))
        // log2(10) is approximately 3.321928094887362
        mpfr_prec_t precision_bits = static_cast<mpfr_prec_t>(std::ceil(required_dps * std::log2(10.0)));
        if (precision_bits < MPFR_PREC_MIN) precision_bits = MPFR_PREC_MIN;


        // Initialize MPFR variable with required precision
        mpfr_t pi_val;
        mpfr_init2(pi_val, precision_bits);

        // Calculate Pi
        mpfr_const_pi(pi_val, MPFR_RNDN); // MPFR_RNDN is round to nearest

        // Convert MPFR value to string
        // We need total_written + remaining_digits digits after the decimal.
        // mpfr_get_str(..., n_digits, ...) requests n_digits *after* the radix point.
        // The resulting string will have n_digits + 1 characters (digit before + n_digits after).
        // The exponent indicates the position of the radix point. For 3.14..., exponent is 1.
        // The string will be "314159..."
        size_t num_digits_to_get = required_dps; // Number of digits *after* the decimal point to get

        // mpfr_get_str returns a string allocated by GMP/MPFR
        // The exponent returned indicates the position of the decimal point.
        // For 3.14..., base 10, exponent will be 1 (meaning 3 * 10^1).
        // The string will be "314159..."
        mp_exp_t exponent;
        char* pi_str = mpfr_get_str(NULL, &exponent, 10, num_digits_to_get, pi_val, MPFR_RNDN);

        if (pi_str == NULL) {
            std::cerr << "Error converting MPFR value to string." << std::endl;
            mpfr_clear(pi_val);
            break; // Exit loop on error
        }

        // Extract the required substring
        // Python slice [total_written + 2 : total_written + 2 + remaining_digits]
        // str(mp.pi) is like "3.14159..."
        // Index 0: '3', Index 1: '.', Index 2: '1' (first digit after decimal)
        // The slice starts at index total_written + 2. This corresponds to the total_written-th digit AFTER the decimal.
        // mpfr_get_str gives "314159..." with exponent 1.
        // The digits after the decimal start at index 1 of pi_str.
        // We need remaining_digits characters starting from index total_written + 1.
        std::string pi_value_str(pi_str);
        std::string pi_chunk;

        // The exponent tells us where the decimal point *would* be if the string was infinite.
        // For 3.14..., exponent is 1. The string is "314159...".
        // The digit at index 0 is the one before the decimal.
        // The digits after the decimal start at index 1.
        // We need the chunk starting from the total_written-th digit *after* the decimal.
        // This is at index 1 + total_written in the pi_str.
        size_t start_index_in_str = 1 + total_written; // 1 for the digit before the decimal

        // Ensure we don't try to extract beyond the available digits in pi_str
        size_t available_digits_in_str = pi_value_str.length();

        if (start_index_in_str >= available_digits_in_str) {
             // This can happen if the requested precision was too low to generate
             // total_written + remaining_digits digits after the decimal.
             // mpfr_get_str(..., required_dps, ...) requests required_dps digits *after* the point.
             // The resulting string has length required_dps + 1.
             // So available_digits_in_str should be required_dps + 1.
             // start_index_in_str is 1 + total_written.
             // We need 1 + total_written < required_dps + 1
             // total_written < required_dps
             // total_written < total_written + remaining_digits
             // 0 < remaining_digits. This condition is checked earlier.
             // This check might still be useful if mpfr_get_str behaves unexpectedly.
             std::cerr << "Error: Not enough digits generated by MPFR. Requested " << required_dps << " dps after point, got string length " << available_digits_in_str << ". Attempted start index: " << start_index_in_str << std::endl;
             mpfr_free_str(pi_str); // Free the allocated string
             mpfr_clear(pi_val);
             break; // Exit loop
        }

        size_t length_to_extract = std::min((size_t)remaining_digits, available_digits_in_str - start_index_in_str);

        pi_chunk = pi_value_str.substr(start_index_in_str, length_to_extract);


        // Write the chunk to the file
        file_a << pi_chunk;
        file_a.flush(); // Ensure data is written immediately

        // Update total_written
        total_written += length_to_extract; // Use length_to_extract in case less was available

        // Free the allocated string
        mpfr_free_str(pi_str);

        // Clear MPFR variable
        mpfr_clear(pi_val);

        // Get and print system usage
        std::pair<double, double> usage = get_system_usage();
        double cpu_usage = usage.first;
        double ram_usage = usage.second;

        // Print progress using stringstream for formatting
        std::stringstream progress_ss;
        progress_ss << "Written " << total_written << "/" << (is_infinite ? "inf" : std::to_string(total_digits_ll)) << " digits to file... | CPU Usage: " << std::fixed << std::setprecision(1) << cpu_usage << "% | RAM Usage: " << std::fixed << std::setprecision(1) << ram_usage << "%";
        std::cout << progress_ss.str() << std::endl;

        // If not infinite and we've written enough, break
        if (!is_infinite && total_written >= total_digits_ll) {
            break;
        }
    }

    // Close the file stream
    file_a.close();

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time);
    double total_time = total_time_span.count();

    long long minutes = static_cast<long long>(total_time) / 60;
    long long seconds = static_cast<long long>(total_time) % 60;

    std::cout << "Calculation complete. " << total_written << " digits written to " << file_path << " in " << minutes << " minutes " << seconds << " seconds." << std::endl;
}

int main() {
    std::string output_file = "pi_digits.txt";
    // Total digits to print
    // Use std::numeric_limits<double>::infinity() for infinite calculation
    double total_digits = std::numeric_limits<double>::infinity(); // Default is infinity

    calculate_pi_to_file(output_file, total_digits);

    return 0;
}
