#include "LimitOrderBook.h"
#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include <unordered_set>
#include <iostream>
#include <cmath>

// Ladder parameters mirrored for tests (match your LOB constants)
static constexpr double MIN_PRICE = 90.0;
static constexpr double MAX_PRICE = 110.0;
static constexpr double TICK = 0.01;

// Helpers
static inline std::size_t price_to_index(double price) {
    // Use llround to be robust to floating point (we only use 2dp prices)
    return static_cast<std::size_t>(std::llround((price - MIN_PRICE) / TICK));
}
static inline double index_to_price(std::size_t idx) {
    return MIN_PRICE + idx * TICK;
}
static inline double quantize(double p) {
    return std::round(p / TICK) * TICK;
}
static int32_t level_total_by_side(const PriceLevel& pl, OrderSide side) {
    int32_t sum = 0;
    for (auto* o : pl.orders) if (o->side == side) sum += o->quantity;
    return sum;
}

// --- Basic Operations ---

TEST(LimitOrderBookTest, AddOrderInsertsCorrectly) {
    LimitOrderBook lob;
    lob.process_order(1, 100.00, 100, OrderSide::Buy);

    const auto& levels = lob.get_price_levels();
    std::size_t idx = price_to_index(100.00);
    ASSERT_LT(idx, levels.size());
    EXPECT_EQ(levels[idx].total_quantity, 100);
    EXPECT_EQ(level_total_by_side(levels[idx], OrderSide::Buy), 100);
}

TEST(LimitOrderBookTest, MatchBuyAgainstSell) {
    LimitOrderBook lob;
    lob.process_order(1, 100.00, 100, OrderSide::Buy);
    lob.process_order(2, 100.00,  50, OrderSide::Sell); // should match fully

    const auto& levels = lob.get_price_levels();
    std::size_t idx = price_to_index(100.00);
    ASSERT_LT(idx, levels.size());
    EXPECT_EQ(levels[idx].total_quantity, 50); // 100 - 50
    EXPECT_EQ(level_total_by_side(levels[idx], OrderSide::Buy), 50);
    EXPECT_EQ(level_total_by_side(levels[idx], OrderSide::Sell), 0);
}

TEST(LimitOrderBookTest, CancelRemovesOrder) {
    LimitOrderBook lob;
    lob.process_order(1, 100.00, 100, OrderSide::Buy);
    lob.cancel_order(1);

    const auto& levels = lob.get_price_levels();
    std::size_t idx = price_to_index(100.00);
    EXPECT_EQ(levels[idx].total_quantity, 0);
    EXPECT_TRUE(levels[idx].orders.empty());
}

TEST(LimitOrderBookTest, ModifyUpdatesQuantity) {
    LimitOrderBook lob;
    lob.process_order(1, 100.00, 100, OrderSide::Buy);
    lob.modify_order(1, 150);

    const auto& levels = lob.get_price_levels();
    std::size_t idx = price_to_index(100.00);
    ASSERT_LT(idx, levels.size());
    EXPECT_EQ(levels[idx].total_quantity, 150);
    EXPECT_EQ(level_total_by_side(levels[idx], OrderSide::Buy), 150);
}

// --- Edge Cases & Multi-Level Scenarios ---

TEST(LimitOrderBookTest, BuyOrderSweepsMultipleAsks) {
    LimitOrderBook lob;

    // Add asks at increasing prices
    lob.process_order(1, 101.00,  50, OrderSide::Sell);
    lob.process_order(2, 102.00,  75, OrderSide::Sell);
    lob.process_order(3, 103.00, 100, OrderSide::Sell);

    // Large buy order should sweep across levels
    lob.process_order(4, 103.00, 200, OrderSide::Buy);

    const auto& levels = lob.get_price_levels();
    // 50 + 75 + (100 -> 75 filled) leaves 25 at 103.00
    std::size_t idx103 = price_to_index(103.00);
    ASSERT_LT(idx103, levels.size());
    EXPECT_EQ(levels[idx103].total_quantity, 25);
    // Lower ask levels emptied
    EXPECT_EQ(levels[price_to_index(101.00)].total_quantity, 0);
    EXPECT_EQ(levels[price_to_index(102.00)].total_quantity, 0);
}

TEST(LimitOrderBookTest, PartialFillLeavesRestingOrder) {
    LimitOrderBook lob;

    lob.process_order(1, 101.00, 100, OrderSide::Sell); // resting sell
    lob.process_order(2, 101.00,  40, OrderSide::Buy);  // incoming buy smaller

    const auto& levels = lob.get_price_levels();
    std::size_t idx = price_to_index(101.00);
    ASSERT_LT(idx, levels.size());
    EXPECT_EQ(levels[idx].total_quantity, 60); // 100 - 40
    EXPECT_EQ(level_total_by_side(levels[idx], OrderSide::Sell), 60);
}

TEST(LimitOrderBookTest, PriceLevelIsRemovedWhenEmpty) {
    LimitOrderBook lob;

    lob.process_order(1, 101.00, 50, OrderSide::Sell);
    lob.process_order(2, 101.00, 50, OrderSide::Buy); // matches fully

    const auto& levels = lob.get_price_levels();
    std::size_t idx = price_to_index(101.00);
    EXPECT_EQ(levels[idx].total_quantity, 0);
    EXPECT_TRUE(levels[idx].orders.empty());
}

TEST(LimitOrderBookTest, TotalQuantityMatchesOrders) {
    LimitOrderBook lob;

    lob.process_order(1, 100.00, 40, OrderSide::Buy);
    lob.process_order(2, 100.00, 60, OrderSide::Buy);

    const auto& levels = lob.get_price_levels();
    std::size_t idx = price_to_index(100.00);
    ASSERT_LT(idx, levels.size());

    // Sum orders manually
    int sum = 0;
    for (auto* o : levels[idx].orders) sum += o->quantity;
    EXPECT_EQ(sum, levels[idx].total_quantity);

    // cancel one order → should update correctly
    lob.cancel_order(1);
    int sum2 = 0;
    for (auto* o : levels[idx].orders) sum2 += o->quantity;
    EXPECT_EQ(sum2, levels[idx].total_quantity);
    EXPECT_EQ(levels[idx].total_quantity, 60);
}

TEST(LimitOrderBookTest, ModifyAfterPartialFill) {
    LimitOrderBook lob;

    lob.process_order(1, 100.00, 100, OrderSide::Buy);
    lob.process_order(2, 100.00,  60, OrderSide::Sell); // matches partially → order 1 left with 40

    lob.modify_order(1, 80); // increase from 40 to 80

    const auto& levels = lob.get_price_levels();
    std::size_t idx = price_to_index(100.00);
    ASSERT_LT(idx, levels.size());
    EXPECT_EQ(levels[idx].total_quantity, 80);
    EXPECT_EQ(level_total_by_side(levels[idx], OrderSide::Buy), 80);
}

TEST(LimitOrderBookTest, SameSideDoesNotMatch) {
    LimitOrderBook lob;
    lob.process_order(1, 100.00, 40, OrderSide::Buy);
    lob.process_order(2, 100.00, 60, OrderSide::Buy); // must NOT match order 1

    const auto& levels = lob.get_price_levels();
    auto idx = static_cast<size_t>((100.00 - 90.0) / 0.01);
    ASSERT_EQ(levels[idx].total_quantity, 100);
}

// --- Stress Testing ---

TEST(LimitOrderBookStressTest, RandomizedOperationsWithTiming) {
    LimitOrderBook lob;
    std::mt19937 rng(42); // fixed seed for reproducibility

    // Random price in [90.00, 110.00] then quantize to 0.01
    std::uniform_real_distribution<double> price_dist(MIN_PRICE, MAX_PRICE);
    std::uniform_int_distribution<int32_t> qty_dist(1, 200);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> op_dist(0, 9); // 0-6 = add, 7 = cancel, 8 = modify, 9 = no-op

    std::unordered_set<int64_t> active_ids;
    int64_t next_order_id = 1;

    const int NUM_OPS = 100000; // scale this up later

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_OPS; i++) {
        int op = op_dist(rng);

        if (op <= 6) {
            // Add order
            int64_t id = next_order_id++;
            double price = quantize(price_dist(rng));
            int32_t qty = qty_dist(rng);
            OrderSide side = side_dist(rng) == 0 ? OrderSide::Buy : OrderSide::Sell;

            // Ensure price is in-range after quantization
            if (price < MIN_PRICE) price = MIN_PRICE;
            if (price > MAX_PRICE) price = MAX_PRICE;

            lob.process_order(id, price, qty, side);
            active_ids.insert(id);
        } else if (op == 7 && !active_ids.empty()) {
            // Cancel random order
            auto it = active_ids.begin();
            std::advance(it, rng() % active_ids.size());
            lob.cancel_order(*it);
            active_ids.erase(it);
        } else if (op == 8 && !active_ids.empty()) {
            // Modify random order
            auto it = active_ids.begin();
            std::advance(it, rng() % active_ids.size());
            int32_t new_qty = qty_dist(rng);
            lob.modify_order(*it, new_qty);
        }
        // op == 9 → no-op
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    double ops_per_sec = (NUM_OPS / elapsed_ms) * 1000.0;

    std::cout << "Performed " << NUM_OPS << " operations in " << elapsed_ms << " ms ("
              << ops_per_sec << " ops/sec)\n";

    // --- Invariant checks over the vector ladder ---
    const auto& levels = lob.get_price_levels();
    for (std::size_t i = 0; i < levels.size(); ++i) {
        const auto& pl = levels[i];
        int sum = 0;
        for (auto* order : pl.orders) {
            sum += order->quantity;
            // Also assert price is consistent with ladder slot
            // (Optional sanity): orders are allowed to share level by price only
        }
        EXPECT_EQ(sum, pl.total_quantity) << "at price " << index_to_price(i);
        if (pl.orders.empty()) {
            EXPECT_EQ(pl.total_quantity, 0) << "Non-zero total at empty level " << index_to_price(i);
        }
    }
}
