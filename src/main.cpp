#include <iostream>
#include "LimitOrderBook.h"

void print_book(const LimitOrderBook& lob) {
    std::cout << "\n--- Current Bids ---\n";
    for (const auto& pair : lob.get_bids()) {
        const auto& price = pair.first;
        const auto& pl = pair.second;
        std::cout << "Price: " << price << ", Qty: " << pl->total_quantity << " | Orders: ";
        for (auto* order : pl->orders) {
            std::cout << "(" << order->order_id << ", " << order->quantity << ") ";
        }
        std::cout << "\n";
    }

    std::cout << "\n--- Current Asks ---\n";
    for (const auto& pair : lob.get_asks()) {
        const auto& price = pair.first;
        const auto& pl = pair.second;
        std::cout << "Price: " << price << ", Qty: " << pl->total_quantity << " | Orders: ";
        for (auto* order : pl->orders) {
            std::cout << "(" << order->order_id << ", " << order->quantity << ") ";
        }
        std::cout << "\n";
    }
}

int main() {
    LimitOrderBook lob;

    std::cout << "=== Add initial orders ===\n";
    lob.process_order(1, 1000, 100, OrderSide::Buy);   // bid
    lob.process_order(2, 1010, 50, OrderSide::Buy);    // bid
    lob.process_order(3, 1020, 75, OrderSide::Sell);   // ask
    lob.process_order(4, 1030, 120, OrderSide::Sell);  // ask
    print_book(lob);

    std::cout << "\n=== Add crossing order (Buy 80 @ 1030) ===\n";
    lob.process_order(5, 1030, 80, OrderSide::Buy);    // should match with order 3 (75 @ 1020) + partially with order 4
    print_book(lob);

    std::cout << "\n=== Add crossing order (Sell 120 @ 1000) ===\n";
    lob.process_order(6, 1000, 120, OrderSide::Sell);  // should match with order 2 (50 @ 1010) + order 1 (100 @ 1000), leaving 30
    print_book(lob);

    std::cout << "\n=== Cancel an order (order 4 if still alive) ===\n";
    lob.cancel_order(4);
    print_book(lob);

    std::cout << "\n=== Modify an order (reduce order 1 to 50 if still alive) ===\n";
    lob.modify_order(1, 50);
    print_book(lob);

    std::cout << "\n=== Modify an order (increase order 2 to 200 if still alive) ===\n";
    lob.modify_order(2, 200);
    print_book(lob);

    return 0;
}
