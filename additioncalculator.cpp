#include <iostream>

int add(int a, int b) {
    return a + b;
}

int main() {
  // change the "#, #" in the "int result = add(#, #); on line 9
    int result = add(3, 4);
    std::cout << "Result: " << result << std::endl;
    
    return 0;
}
