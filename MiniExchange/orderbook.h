#pragma once

#include "Order.h"

#include <functional>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class OrderBook
{
private:
    using OrderList = std::deque<Order>;

    // Highest bid first.
    std::map<
        long long,
        OrderList,
        std::greater<long long>
    > bids;

    // Lowest ask first.
    std::map<
        long long,
        OrderList
    > asks;

    /*
        Fast lookup table.

        Instead of searching the entire order book for an ID,
        this tells us:

            order ID
                ↓
            side
            price
            iterator to exact Order

        std::list iterators remain valid when other elements
        are inserted or erased.
    */
    struct OrderLocation
    {
        Side side;
        long long price;
    };

    std::unordered_map<
        long long,
        OrderLocation
    > orderIndex;

    std::string symbol;
    std::vector<Account>& accounts;

    bool verbose = true;

    int tradeCount = 0;
    long long totalVolume = 0;
    long long totalValue = 0;

    long long lastTradePrice = 0;
    long long highPrice = 0;
    long long lowPrice = 0;

    void executeTrade(
        Order& buy,
        Order& sell,
        int quantity,
        long long tradePrice,
        bool buyCashAlreadyReserved
    );

    void addLimitBuy(Order incoming);
    void addLimitSell(Order incoming);

    void addMarketBuy(Order incoming);
    void addMarketSell(Order incoming);

    // Add a resting order to the index.
    void indexOrder(
        long long orderId,
        Side side,
        long long price
    );

    // Remove an order from the index.
    void unindexOrder(long long orderId);

public:
    OrderBook(
        const std::string& symbol,
        std::vector<Account>& accounts
    );

    void addOrder(Order order);

    /*
        Cancel an existing resting order.

        Average lookup:
            O(1)

        Erasing from std::list:
            O(1)
    */
    bool cancelOrder(long long orderId);

    /*
        Replace an existing order.

        For simplicity and predictable price-time priority,
        replacement works as:

            cancel old order
            +
            submit replacement

        Therefore a modified order loses its old time priority.
    */
    bool replaceOrder(
        long long orderId,
        int newQuantity,
        long long newPrice
    );

    void printBook() const;
    void printStatistics() const;

    void cancelRemainingOrders();

    void setVerbose(bool value);

    int getTradeCount() const;
    long long getTotalVolume() const;
    long long getLastTradePrice() const;

    int getOpenOrderCount() const;
};