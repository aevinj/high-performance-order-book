#include "LimitOrderBook.h"
#include <iostream>

void LimitOrderBook::add_order(int64_t order_id, int64_t price, int32_t quantity, OrderSide side) {
    // Create the new order object
    auto new_order = std::make_unique<Order>(Order{order_id, price, quantity, side});

    // Simple logic for a non-matching order (no trades)
    if (side == OrderSide::Buy) {
        auto& price_level = bids[price];
        if (!price_level) {
            // If the price level doesn't exist, create it
            price_level = std::make_unique<PriceLevel>();
        }
        price_level->orders.push_back(std::move(new_order));
        price_level->total_quantity += quantity;
    } else { // side == OrderSide::Sell
        auto& price_level = asks[price];
        if (!price_level) {
            // If the price level doesn't exist, create it
            price_level = std::make_unique<PriceLevel>();
        }
        price_level->orders.push_back(std::move(new_order));
        price_level->total_quantity += quantity;
    }

    std::cout << "Added order " << order_id << " at price " << price << " with quantity " << quantity << std::endl;
}

void LimitOrderBook::cancel_order(int64_t order_id) {
    // TODO: Implement this method.
    // It requires a way to quickly look up an order by its ID.
    // A simple way would be to iterate through all bids and asks,
    // but a better way is to use a std::unordered_map<int64_t, Order*>
    // for quick lookup. We will add this later.
    std::cout << "Cancel functionality is not yet implemented." << std::endl;
}

void LimitOrderBook::modify_order(int64_t order_id, int32_t new_quantity) {
    // TODO: Implement this method.
    std::cout << "Modify functionality is not yet implemented." << std::endl;
}