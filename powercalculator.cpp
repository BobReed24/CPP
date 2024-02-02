#include <iostream>
#include <cmath>

int main() {
    double base, exponent, result;

    // Get user input for base
    std::cout << "Enter the base: ";
    std::cin >> base;

    // Get user input for exponent
    std::cout << "Enter the exponent: ";
    std::cin >> exponent;

    // Calculate the power using the pow function from cmath
    result = std::pow(base, exponent);

    // Display the result
    std::cout << base << " raised to the power of " << exponent << " is: " << result << std::endl;

    return 0;
}
