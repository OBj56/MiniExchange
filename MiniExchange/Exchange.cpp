#include "Exchange.h"

#include <iostream>

Exchange::Exchange(std::vector<Account>& accounts)
    : accounts(accounts)
{}

void Exchange::addSymbol(const std::string& symbol)
{
    if (books.contains(symbol))
        return;

    books[symbol] =
        std::make_unique<OrderBook>(
            symbol,
            accounts
        );
}

void Exchange::addOrder(Order order)
{
    auto it = books.find(order.symbol);

    if (it == books.end())
    {
        std::cout
            << "Unknown symbol: "
            << order.symbol
            << '\n';

        return;
    }

    it->second->addOrder(order);
}

bool Exchange::cancelOrder(
    const std::string& symbol,
    long long orderId
)
{
    auto it = books.find(symbol);

    if (it == books.end())
        return false;

    return it->second->cancelOrder(orderId);
}

OrderBook* Exchange::getBook(
    const std::string& symbol
)
{
    auto it = books.find(symbol);

    if (it == books.end())
        return nullptr;

    return it->second.get();
}

void Exchange::cancelAllRemainingOrders()
{
    for (auto& [symbol, book] : books)
    {
        book->cancelRemainingOrders();
    }
}

bool Exchange::replaceOrder(
    const std::string& symbol,
    long long orderId,
    int newQuantity,
    long long newPrice
)
{
    auto it = books.find(symbol);

    if (it == books.end())
        return false;

    return it->second->replaceOrder(
        orderId,
        newQuantity,
        newPrice
    );
}