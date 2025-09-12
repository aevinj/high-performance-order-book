#include <iostream>
#include "LimitOrderBook.h" // Include your new header file

int main() {
    LimitOrderBook lob;

    std::cout << "Adding orders..." << std::endl;
    lob.add_order(1, 1000, 100, OrderSide::Buy);
    lob.add_order(2, 1000, 50, OrderSide::Buy);
    lob.add_order(3, 1010, 200, OrderSide::Buy);
    lob.add_order(4, 1020, 75, OrderSide::Sell);
    lob.add_order(5, 1020, 150, OrderSide::Sell);
    lob.add_order(6, 1030, 120, OrderSide::Sell);

    std::cout << "\n--- Current Bids ---" << std::endl;
    for (const auto& pair : lob.get_bids()) {
        const auto& price = pair.first;
        const auto& price_level = pair.second;
        std::cout << "Price: " << price << ", Total Quantity: " << price_level->total_quantity << ", Orders: ";
        for (const auto& order : price_level->orders) {
            std::cout << "(" << order->order_id << ", " << order->quantity << ") ";
        }
        std::cout << std::endl;
    }

    std::cout << "\n--- Current Asks ---" << std::endl;
    for (const auto& pair : lob.get_asks()) {
        const auto& price = pair.first;
        const auto& price_level = pair.second;
        std::cout << "Price: " << price << ", Total Quantity: " << price_level->total_quantity << ", Orders: ";
        for (const auto& order : price_level->orders) {
            std::cout << "(" << order->order_id << ", " << order->quantity << ") ";
        }
        std::cout << std::endl;
    }

    return 0;
}