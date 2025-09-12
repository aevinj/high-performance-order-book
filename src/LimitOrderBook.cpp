#include "LimitOrderBook.h"
#include <iostream>

void LimitOrderBook::add_order(int64_t order_id, int64_t price, int32_t quantity, OrderSide side) {
    // Create the new order object
    std::unique_ptr<Order>& order_ref = orders_by_id[order_id]; // allocates if doesn't exist
    order_ref = std::make_unique<Order>(Order{order_id, price, quantity, side});
    Order* new_order_ptr = order_ref.get();

    // Simple logic for a non-matching order (no trades)
    if (side == OrderSide::Buy) {
        auto& price_level = bids[price];
        if (!price_level) {
            // If the price level doesn't exist, create it
            price_level = std::make_unique<PriceLevel>();
        }
        price_level->orders.push_back(new_order_ptr);
        price_level->total_quantity += quantity;
    } else { // side == OrderSide::Sell
        auto& price_level = asks[price];
        if (!price_level) {
            // If the price level doesn't exist, create it
            price_level = std::make_unique<PriceLevel>();
        }
        price_level->orders.push_back(new_order_ptr);
        price_level->total_quantity += quantity;
    }

    std::cout << "Added order " << order_id << " at price " << price << " with quantity " << quantity << std::endl;
}

void LimitOrderBook::cancel_order(int64_t order_id) {
    // Step 1: Find the order using .find() and get its iterator.
    auto it = orders_by_id.find(order_id);
    if (it == orders_by_id.end()) {
        std::cout << "Could not find order: " << order_id << std::endl;
        return; // Return immediately if not found
    }

    // Now, get the unique_ptr and the raw pointer from the iterator.
    // This is the correct way to get the data without a second lookup.
    const auto& order_unique_ptr = it->second;
    int64_t price = order_unique_ptr->price;
    OrderSide side = order_unique_ptr->side;
    int32_t quantity = order_unique_ptr->quantity;

    // This is the core logic. Restructure the if/else to avoid the scope bug.
    if (side == OrderSide::Buy) {
        // Find the price level in the bids map
        auto price_level_it = bids.find(price);
        if (price_level_it != bids.end()) {
            // Get a reference to the PriceLevel's unique_ptr
            auto& price_level_unique_ptr = price_level_it->second;

            // Step 2: Decrement the quantity
            price_level_unique_ptr->total_quantity -= quantity;

            // Step 3: Remove the raw pointer from the list.
            // Use a loop with an iterator to find the pointer.
            auto& orders_list = price_level_unique_ptr->orders;
            for (auto list_it = orders_list.begin(); list_it != orders_list.end(); ++list_it) {
                if (*list_it == order_unique_ptr.get()) {
                    orders_list.erase(list_it);
                    break;
                }
            }

            // Optional: If the price level is now empty, remove it from the map.
            if (orders_list.empty()) {
                bids.erase(price_level_it);
            }
        }
    } else { // OrderSide::Sell
        auto price_level_it = asks.find(price);
        if (price_level_it != asks.end()) {
            auto& price_level_unique_ptr = price_level_it->second;
            price_level_unique_ptr->total_quantity -= quantity;

            auto& orders_list = price_level_unique_ptr->orders;
            for (auto list_it = orders_list.begin(); list_it != orders_list.end(); ++list_it) {
                if (*list_it == order_unique_ptr.get()) {
                    orders_list.erase(list_it);
                    break;
                }
            }
            if (orders_list.empty()) {
                bids.erase(price_level_it);
            }
        }
    }

    // Step 4: Erase the unique_ptr from the orders_by_id map to deallocate memory.
    orders_by_id.erase(it);

    std::cout << "Successfully canceled order: " << order_id << std::endl;
}

void LimitOrderBook::modify_order(int64_t order_id, int32_t new_quantity) {
    auto order_by_id_it = orders_by_id.find(order_id);
    if (order_by_id_it == orders_by_id.end()) {
        std::cout << "Could not find order: " << order_id << std::endl;
        return; // Return immediately if not found
    }

    if (new_quantity <= 0) {
        std::cout << "Quantity must be positive. Cancelling order " << order_id << " instead." << std::endl;
        cancel_order(order_id);
        return;
    }

    auto& order_ptr = order_by_id_it->second;
    auto diff = new_quantity - order_ptr->quantity;
    order_ptr->quantity += diff;
    if (order_ptr->side == OrderSide::Buy) {
        auto price_level_it = bids.find(order_ptr->price);
        if (price_level_it != bids.end()) {
            auto& price_level_unique_ptr = price_level_it->second;
            price_level_unique_ptr->total_quantity += diff;
        }
    } else {
        auto price_level_it = asks.find(order_ptr->price);
        if (price_level_it != asks.end()) {
            auto& price_level_unique_ptr = price_level_it->second;
            price_level_unique_ptr->total_quantity += diff;
        }
    }
}