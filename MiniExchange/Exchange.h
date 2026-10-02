#pragma once

#include "OrderBook.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Exchange
{
private:
    std::vector<Account>& accounts;

    std::unordered_map<
        std::string,
        std::unique_ptr<OrderBook>
    > books;

public:
    explicit Exchange(std::vector<Account>& accounts);

    void addSymbol(const std::string& symbol);

    void addOrder(Order order);

    bool cancelOrder(
        const std::string& symbol,
        long long orderId
    );

    bool replaceOrder(
        const std::string& symbol,
        long long orderId,
        int newQuantity,
        long long newPrice
    );

    OrderBook* getBook(const std::string& symbol);

    void cancelAllRemainingOrders();
};