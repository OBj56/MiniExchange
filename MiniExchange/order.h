#pragma once

#include <string>
#include <unordered_map>

enum class Side
{
    Buy,
    Sell
};

enum class OrderType
{
    Limit,
    Market
};

struct Order
{
    long long id;
    std::string symbol;
    Side side;
    OrderType type;
    int quantity;
    long long price; // cents; ignored for market orders
    int traderid;
};

struct Account
{
    std::string name;
    long long cash; // cents

    // Number of shares owned for each symbol.
    std::unordered_map<std::string, int> shares;
};

inline std::string dollars(long long cents)
{
    bool negative = cents < 0;

    if (negative)
        cents = -cents;

    std::string result =
        "$" +
        std::to_string(cents / 100) +
        "." +
        (cents % 100 < 10 ? "0" : "") +
        std::to_string(cents % 100);

    return negative ? "-" + result : result;
}