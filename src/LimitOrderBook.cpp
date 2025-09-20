#include "LimitOrderBook.h"
#include <iostream>

void LimitOrderBook::process_order(int64_t order_id, int64_t price, int32_t quantity, OrderSide side) {
    Order* new_order_ptr = order_pool.allocate();
    *new_order_ptr = Order{order_id, price, quantity, side};
    orders_by_id[order_id] = new_order_ptr;
    
    // try to match
    match(new_order_ptr);

    // if still has quantity, insert into book
    if (new_order_ptr->quantity > 0) {
        insert_order(new_order_ptr);
    } else {
        orders_by_id.erase(order_id);
        order_pool.deallocate(new_order_ptr);
    }
}

void LimitOrderBook::match(Order* incoming) {
    if (incoming->side == OrderSide::Buy) {
        // BUY should match only against resting SELLS at prices <= incoming->price
        for (size_t idx = 0; idx < NUM_LEVELS && incoming->quantity > 0; ++idx) {
            double level_price = MIN_PRICE + idx * TICK_SIZE;
            if (level_price > incoming->price) break;

            auto& level = price_levels[idx];
            auto& q = level.orders;

            for (auto it = q.begin(); it != q.end() && incoming->quantity > 0; ) {
                Order* resting = *it;
                if (resting->side != OrderSide::Sell) { // filter same-side orders
                    ++it;
                    continue;
                }

                int32_t trade_qty = std::min(incoming->quantity, resting->quantity);
                incoming->quantity -= trade_qty;
                resting->quantity  -= trade_qty;
                level.total_quantity -= trade_qty;

                if (resting->quantity == 0) {
                    orders_by_id.erase(resting->order_id);
                    order_pool.deallocate(resting);
                    it = q.erase(it);
                } else {
                    ++it;
                }
            }
        }
    } else {
        // SELL should match only against resting BUYS at prices >= incoming->price
        for (size_t idx = NUM_LEVELS - 1; idx < NUM_LEVELS && incoming->quantity > 0; --idx) {
            double level_price = MIN_PRICE + idx * TICK_SIZE;
            if (level_price < incoming->price) break;

            auto& level = price_levels[idx];
            auto& q = level.orders;

            for (auto it = q.begin(); it != q.end() && incoming->quantity > 0; ) {
                Order* resting = *it;
                if (resting->side != OrderSide::Buy) { // filter same-side orders
                    ++it;
                    continue;
                }

                int32_t trade_qty = std::min(incoming->quantity, resting->quantity);
                incoming->quantity -= trade_qty;
                resting->quantity  -= trade_qty;
                level.total_quantity -= trade_qty;

                if (resting->quantity == 0) {
                    orders_by_id.erase(resting->order_id);
                    order_pool.deallocate(resting);
                    it = q.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}


void LimitOrderBook::insert_order(Order* incoming) {
    // The price_levels vector will only ever store one side at a time - if there
    // was a buy and sell at 1 price level, it would've already matched -- its basc
    // a backlog of orders waiting to be matched
    size_t incoming_idx = price_to_index(incoming->price);
    price_levels[incoming_idx].orders.push_back(incoming);
    price_levels[incoming_idx].total_quantity += incoming->quantity;
}

void LimitOrderBook::cancel_order(int64_t order_id) {
    auto it = orders_by_id.find(order_id);
    if (it == orders_by_id.end()) {
        // std::cout << "Could not find order: " << order_id << std::endl;
        return;
    }

    Order* order_ptr = it->second;
    size_t idx = price_to_index(order_ptr->price);

    // Adjust total
    price_levels[idx].total_quantity -= order_ptr->quantity;

    // Remove from queue
    auto& orders_queue = price_levels[idx].orders;
    for (auto q_it = orders_queue.begin(); q_it != orders_queue.end(); ++q_it) {
        if ((*q_it)->order_id == order_id) {
            orders_queue.erase(q_it);
            break;
        }
    }

    // free + erase
    orders_by_id.erase(it);
    order_pool.deallocate(order_ptr);
}

void LimitOrderBook::modify_order(int64_t order_id, int32_t new_quantity) {
    auto order_by_id_it = orders_by_id.find(order_id);
    if (order_by_id_it == orders_by_id.end()) {
        // std::cout << "Could not find order: " << order_id << std::endl;
        return; // Return immediately if not found
    }

    if (new_quantity <= 0) {
        std::cout << "Quantity must be positive. Cancelling order " << order_id << " instead." << std::endl;
        cancel_order(order_id);
        return;
    }

    auto& order_ptr = order_by_id_it->second;
    auto diff = new_quantity - order_ptr->quantity;
    order_ptr->quantity = new_quantity;

    size_t idx = price_to_index(order_ptr->price);
    price_levels[idx].total_quantity += diff;
}