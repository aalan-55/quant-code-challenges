#include <string>
#include <vector>
#include <iostream>

#include "solution.hpp"

int main() {
    any container = 42; 
    std::cout << "Contains int: " << any_cast<int>(container) << '\n';

    container = std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    auto* vec = any_cast<std::vector<int>>(&container);
    if (vec) {
        std::cout << "Vector size: " << vec->size() << '\n';
    }
  
    try {
        double value = any_cast<double>(container);
        (void)value;
    } catch (const bad_any_cast& e) {
        std::cout << "Caught expected exception: " << e.what() << '\n';
    }

    container.emplace<std::string>("Type erasure with SOO");
    std::cout << "String value: " << any_cast<std::string>(container) << '\n';

    return 0;
}
