#ifndef ORDERBOOK_LIMITORDERBOOK_H
#define ORDERBOOK_LIMITORDERBOOK_H

#include "Order.h"
#include <map>
#include <list>
#include <memory> // For std::unique_ptr

// Represents a collection of orders at a single price level
class PriceLevel {
public:
    // Use a list to maintain Price-Time Priority
    std::list<std::unique_ptr<Order>> orders;
    int32_t total_quantity = 0;
};

class LimitOrderBook {
private:
    // Maps for bids (sorted descending) and asks (sorted ascending)
    std::map<int64_t, std::unique_ptr<PriceLevel>, std::greater<int64_t>> bids;
    std::map<int64_t, std::unique_ptr<PriceLevel>> asks;

public:
    void add_order(int64_t order_id, int64_t price, int32_t quantity, OrderSide side);
    void cancel_order(int64_t order_id);
    void modify_order(int64_t order_id, int32_t new_quantity);
    // Add a getter to view the book for testing
    const std::map<int64_t, std::unique_ptr<PriceLevel>, std::greater<int64_t>>& get_bids() const { return bids; }
    const std::map<int64_t, std::unique_ptr<PriceLevel>>& get_asks() const { return asks; }
};

#endif // ORDERBOOK_LIMITORDERBOOK_H