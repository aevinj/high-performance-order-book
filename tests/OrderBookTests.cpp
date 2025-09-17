#include "LimitOrderBook.h"
#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include <unordered_set>
#include <iostream>

// --- Basic Operations ---

TEST(LimitOrderBookTest, AddOrderInsertsCorrectly) {
    LimitOrderBook lob;
    lob.process_order(1, 1000, 100, OrderSide::Buy);

    auto& bids = lob.get_bids();
    ASSERT_EQ(bids.size(), 1);
    EXPECT_EQ(bids.begin()->first, 1000);
    EXPECT_EQ(bids.begin()->second->total_quantity, 100);
}

TEST(LimitOrderBookTest, MatchBuyAgainstSell) {
    LimitOrderBook lob;
    lob.process_order(1, 1000, 100, OrderSide::Buy);
    lob.process_order(2, 1000, 50, OrderSide::Sell); // should match fully

    auto& bids = lob.get_bids();
    ASSERT_EQ(bids.size(), 1);
    EXPECT_EQ(bids.begin()->second->total_quantity, 50);

    auto& asks = lob.get_asks();
    EXPECT_TRUE(asks.empty());
}

TEST(LimitOrderBookTest, CancelRemovesOrder) {
    LimitOrderBook lob;
    lob.process_order(1, 1000, 100, OrderSide::Buy);
    lob.cancel_order(1);

    EXPECT_TRUE(lob.get_bids().empty());
}

TEST(LimitOrderBookTest, ModifyUpdatesQuantity) {
    LimitOrderBook lob;
    lob.process_order(1, 1000, 100, OrderSide::Buy);
    lob.modify_order(1, 150);

    auto& bids = lob.get_bids();
    ASSERT_EQ(bids.size(), 1);
    EXPECT_EQ(bids.begin()->second->total_quantity, 150);
}

// --- Edge Cases & Multi-Level Scenarios ---

TEST(LimitOrderBookTest, BuyOrderSweepsMultipleAsks) {
    LimitOrderBook lob;

    // Add asks at increasing prices
    lob.process_order(1, 1010, 50, OrderSide::Sell);
    lob.process_order(2, 1020, 75, OrderSide::Sell);
    lob.process_order(3, 1030, 100, OrderSide::Sell);

    // Large buy order should sweep across levels
    lob.process_order(4, 1030, 200, OrderSide::Buy);

    auto& asks = lob.get_asks();
    ASSERT_EQ(asks.size(), 1);                 // only 1030 left
    EXPECT_EQ(asks.begin()->first, 1030);
    EXPECT_EQ(asks.begin()->second->total_quantity, 25); // 100 - 75 = 25 left

    EXPECT_TRUE(lob.get_bids().empty());       // Buy was fully filled
}

TEST(LimitOrderBookTest, PartialFillLeavesRestingOrder) {
    LimitOrderBook lob;

    lob.process_order(1, 1010, 100, OrderSide::Sell); // resting sell
    lob.process_order(2, 1010, 40, OrderSide::Buy);   // incoming buy smaller

    auto& asks = lob.get_asks();
    ASSERT_EQ(asks.size(), 1);
    EXPECT_EQ(asks.begin()->second->total_quantity, 60); // 100 - 40 = 60 left

    EXPECT_TRUE(lob.get_bids().empty()); // buy filled completely
}

TEST(LimitOrderBookTest, PriceLevelIsRemovedWhenEmpty) {
    LimitOrderBook lob;

    lob.process_order(1, 1010, 50, OrderSide::Sell);
    lob.process_order(2, 1010, 50, OrderSide::Buy); // matches fully, removes level

    EXPECT_TRUE(lob.get_asks().empty());
    EXPECT_TRUE(lob.get_bids().empty());
}

TEST(LimitOrderBookTest, TotalQuantityMatchesOrders) {
    LimitOrderBook lob;

    lob.process_order(1, 1000, 40, OrderSide::Buy);
    lob.process_order(2, 1000, 60, OrderSide::Buy);

    auto& bids = lob.get_bids();
    ASSERT_EQ(bids.size(), 1);
    auto& level = bids.begin()->second;
    EXPECT_EQ(level->total_quantity, 100);

    // cancel one order → should update correctly
    lob.cancel_order(1);
    EXPECT_EQ(level->total_quantity, 60);
}

TEST(LimitOrderBookTest, ModifyAfterPartialFill) {
    LimitOrderBook lob;

    lob.process_order(1, 1000, 100, OrderSide::Buy);
    lob.process_order(2, 1000, 60, OrderSide::Sell); // matches partially → order 1 left with 40

    lob.modify_order(1, 80); // increase from 40 to 80

    auto& bids = lob.get_bids();
    ASSERT_EQ(bids.size(), 1);
    EXPECT_EQ(bids.begin()->second->total_quantity, 80);
}

// --- Stress Testing ---

TEST(LimitOrderBookStressTest, RandomizedOperationsWithTiming) {
    LimitOrderBook lob;
    std::mt19937 rng(42); // fixed seed for reproducibility
    std::uniform_int_distribution<int64_t> price_dist(900, 1100);
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
            int64_t price = price_dist(rng);
            int32_t qty = qty_dist(rng);
            OrderSide side = side_dist(rng) == 0 ? OrderSide::Buy : OrderSide::Sell;

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

    // --- Invariant checks (same as before) ---
    auto check_levels = [](const auto& book) {
        for (const auto& [price, level_ptr] : book) {
            int sum = 0;
            for (auto* order : level_ptr->orders) {
                sum += order->quantity;
            }
            EXPECT_EQ(sum, level_ptr->total_quantity) << "at price " << price;
        }
    };
    check_levels(lob.get_bids());
    check_levels(lob.get_asks());

    for (const auto& [price, level_ptr] : lob.get_bids()) {
        EXPECT_FALSE(level_ptr->orders.empty());
    }
    for (const auto& [price, level_ptr] : lob.get_asks()) {
        EXPECT_FALSE(level_ptr->orders.empty());
    }
}