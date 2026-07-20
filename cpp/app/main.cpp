#include <iostream>

#include "logging.hpp"

int main() {
    logging::initialize();

    std::cout << "Starting crypto_trading_platform\n";
    std::cout << "Version: 0.1.0\n";
    std::cout << "Mode: paper\n";
    std::cout << "System startup complete\n";

    return 0;
}