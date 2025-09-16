#include "LimitOrderBook.h"
#include <gtest/gtest.h>

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
