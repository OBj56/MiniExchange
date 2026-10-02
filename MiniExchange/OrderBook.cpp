#include "OrderBook.h"

#include <algorithm>
#include <iostream>

OrderBook::OrderBook(
    const std::string& symbol,
    std::vector<Account>& accounts
)
    : symbol(symbol),
    accounts(accounts)
{}

void OrderBook::setVerbose(bool value)
{
    verbose = value;
}

void OrderBook::indexOrder(
    long long orderId,
    Side side,
    long long price
)
{
    orderIndex[orderId] = {
        side,
        price
    };
}

void OrderBook::unindexOrder(long long orderId)
{
    orderIndex.erase(orderId);
}

void OrderBook::executeTrade(
    Order& buy,
    Order& sell,
    int quantity,
    long long tradePrice,
    bool buyCashAlreadyReserved
)
{
    Account& buyer = accounts[buy.traderid];
    Account& seller = accounts[sell.traderid];

    if (buyCashAlreadyReserved)
    {
        // Buyer reserved money using their limit price.
        // Return any improvement between the limit price
        // and actual execution price.
        buyer.cash +=
            (buy.price - tradePrice) * quantity;
    }
    else
    {
        // Market buy pays at execution time.
        buyer.cash -= tradePrice * quantity;
    }

    buyer.shares[buy.symbol] += quantity;

    // Seller shares were already reserved.
    seller.cash += tradePrice * quantity;

    buy.quantity -= quantity;
    sell.quantity -= quantity;

    ++tradeCount;

    totalVolume += quantity;
    totalValue += tradePrice * quantity;

    lastTradePrice = tradePrice;

    if (highPrice == 0 || tradePrice > highPrice)
        highPrice = tradePrice;

    if (lowPrice == 0 || tradePrice < lowPrice)
        lowPrice = tradePrice;

    if (verbose)
    {
        std::cout
            << "TRADE " << tradeCount
            << ": "
            << quantity
            << ' '
            << symbol
            << " at "
            << dollars(tradePrice)
            << " | "
            << buyer.name
            << " buys from "
            << seller.name
            << '\n';
    }
}

void OrderBook::addOrder(Order incoming)
{
    if (incoming.symbol != symbol ||
        incoming.quantity <= 0 ||
        incoming.traderid < 0 ||
        incoming.traderid >=
        static_cast<int>(accounts.size()))
    {
        if (verbose)
        {
            std::cout
                << "Order #"
                << incoming.id
                << " rejected: invalid order\n";
        }

        return;
    }

    if (incoming.type == OrderType::Limit &&
        incoming.price <= 0)
    {
        if (verbose)
        {
            std::cout
                << "Order #"
                << incoming.id
                << " rejected: invalid price\n";
        }

        return;
    }

    /*
        Don't allow duplicate active IDs.

        Without this, orderIndex[ID] could overwrite
        the location of another active order.
    */
    if (orderIndex.contains(incoming.id))
    {
        if (verbose)
        {
            std::cout
                << "Order #"
                << incoming.id
                << " rejected: duplicate active ID\n";
        }

        return;
    }

    if (verbose)
    {
        std::cout
            << "ORDER #"
            << incoming.id
            << ": "
            << accounts[incoming.traderid].name
            << ' '
            << (
                incoming.side == Side::Buy
                ? "BUY "
                : "SELL "
                )
            << incoming.quantity
            << ' '
            << symbol;

        if (incoming.type == OrderType::Market)
        {
            std::cout << " MARKET\n";
        }
        else
        {
            std::cout
                << " at "
                << dollars(incoming.price)
                << '\n';
        }
    }

    if (incoming.type == OrderType::Market)
    {
        if (incoming.side == Side::Buy)
            addMarketBuy(incoming);
        else
            addMarketSell(incoming);
    }
    else
    {
        if (incoming.side == Side::Buy)
            addLimitBuy(incoming);
        else
            addLimitSell(incoming);
    }
}

void OrderBook::addLimitBuy(Order incoming)
{
    Account& buyer =
        accounts[incoming.traderid];

    long long requiredCash =
        incoming.price * incoming.quantity;

    if (buyer.cash < requiredCash)
    {
        if (verbose)
        {
            std::cout
                << "  -> rejected: insufficient cash\n";
        }

        return;
    }

    // Reserve full limit-order value.
    buyer.cash -= requiredCash;

    while (
        incoming.quantity > 0 &&
        !asks.empty() &&
        incoming.price >= asks.begin()->first
        )
    {
        auto level = asks.begin();

        Order& restingSell =
            level->second.front();

        int quantity =
            std::min(
                incoming.quantity,
                restingSell.quantity
            );

        executeTrade(
            incoming,
            restingSell,
            quantity,
            restingSell.price,
            true
        );

        if (restingSell.quantity == 0)
        {
            long long completedId =
                restingSell.id;

            unindexOrder(completedId);

            level->second.pop_front();
        }

        if (level->second.empty())
            asks.erase(level);
    }

    if (incoming.quantity > 0)
    {
        auto& level =
            bids[incoming.price];

        
        level.push_back(incoming);

        indexOrder(
            incoming.id,
            Side::Buy,
            incoming.price
        );
    }
}

void OrderBook::addLimitSell(Order incoming)
{
    Account& seller =
        accounts[incoming.traderid];

    int& sellerShares =
        seller.shares[incoming.symbol];

    if (sellerShares < incoming.quantity)
    {
        if (verbose)
        {
            std::cout
                << "  -> rejected: insufficient shares\n";
        }

        return;
    }

    // Reserve shares.
    sellerShares -= incoming.quantity;

    while (
        incoming.quantity > 0 &&
        !bids.empty() &&
        incoming.price <= bids.begin()->first
        )
    {
        auto level = bids.begin();

        Order& restingBuy =
            level->second.front();

        int quantity =
            std::min(
                incoming.quantity,
                restingBuy.quantity
            );

        executeTrade(
            restingBuy,
            incoming,
            quantity,
            restingBuy.price,
            true
        );

        if (restingBuy.quantity == 0)
        {
            long long completedId =
                restingBuy.id;

            unindexOrder(completedId);

            level->second.pop_front();
        }

        if (level->second.empty())
            bids.erase(level);
    }

    if (incoming.quantity > 0)
    {
        auto& level =
            asks[incoming.price];

        level.push_back(incoming);

       

        indexOrder(
            incoming.id,
            Side::Sell,
            incoming.price
        );
    }
}

void OrderBook::addMarketBuy(Order incoming)
{
    Account& buyer =
        accounts[incoming.traderid];

    while (
        incoming.quantity > 0 &&
        !asks.empty()
        )
    {
        auto level = asks.begin();

        Order& restingSell =
            level->second.front();

        long long price =
            restingSell.price;

        long long affordable =
            buyer.cash / price;

        if (affordable <= 0)
        {
            if (verbose)
            {
                std::cout
                    << "  -> market buy stopped: "
                    << "insufficient cash\n";
            }

            break;
        }

        int quantity =
            std::min(
                incoming.quantity,
                restingSell.quantity
            );

        quantity =
            std::min(
                quantity,
                static_cast<int>(affordable)
            );

        executeTrade(
            incoming,
            restingSell,
            quantity,
            price,
            false
        );

        if (restingSell.quantity == 0)
        {
            long long completedId =
                restingSell.id;

            unindexOrder(completedId);

            level->second.pop_front();
        }

        if (level->second.empty())
            asks.erase(level);
    }

    if (
        incoming.quantity > 0 &&
        verbose
        )
    {
        std::cout
            << "  -> "
            << incoming.quantity
            << " market-buy shares unfilled\n";
    }

    // Market orders never rest.
}

void OrderBook::addMarketSell(Order incoming)
{
    Account& seller =
        accounts[incoming.traderid];

    int& sellerShares =
        seller.shares[incoming.symbol];

    if (sellerShares <= 0)
    {
        if (verbose)
        {
            std::cout
                << "  -> rejected: insufficient shares\n";
        }

        return;
    }

    if (incoming.quantity > sellerShares)
        incoming.quantity = sellerShares;

    // Reserve shares for the market sell.
    sellerShares -= incoming.quantity;

    while (
        incoming.quantity > 0 &&
        !bids.empty()
        )
    {
        auto level = bids.begin();

        Order& restingBuy =
            level->second.front();

        int quantity =
            std::min(
                incoming.quantity,
                restingBuy.quantity
            );

        executeTrade(
            restingBuy,
            incoming,
            quantity,
            restingBuy.price,
            true
        );

        if (restingBuy.quantity == 0)
        {
            long long completedId =
                restingBuy.id;

            unindexOrder(completedId);

            level->second.pop_front();
        }

        if (level->second.empty())
            bids.erase(level);
    }

    // Return unsold shares.
    if (incoming.quantity > 0)
    {
        sellerShares += incoming.quantity;

        if (verbose)
        {
            std::cout
                << "  -> "
                << incoming.quantity
                << " market-sell shares unfilled\n";
        }
    }

    // Market orders never rest.
}

bool OrderBook::cancelOrder(long long orderId)
{
    auto found = orderIndex.find(orderId);

    if (found == orderIndex.end())
        return false;

    OrderLocation location = found->second;

    if (location.side == Side::Buy)
    {
        auto level = bids.find(location.price);

        if (level == bids.end())
            return false;

        auto& orders = level->second;

        for (auto it = orders.begin();
            it != orders.end();
            ++it)
        {
            if (it->id != orderId)
                continue;

            // Return reserved cash.
            accounts[it->traderid].cash +=
                it->price * it->quantity;

            if (verbose)
            {
                std::cout
                    << "Cancelled BUY order #"
                    << orderId
                    << '\n';
            }

            orders.erase(it);
            orderIndex.erase(found);

            if (orders.empty())
                bids.erase(level);

            return true;
        }
    }
    else
    {
        auto level = asks.find(location.price);

        if (level == asks.end())
            return false;

        auto& orders = level->second;

        for (auto it = orders.begin();
            it != orders.end();
            ++it)
        {
            if (it->id != orderId)
                continue;

            // Return reserved shares.
            accounts[it->traderid]
                .shares[symbol] +=
                it->quantity;

            if (verbose)
            {
                std::cout
                    << "Cancelled SELL order #"
                    << orderId
                    << '\n';
            }

            orders.erase(it);
            orderIndex.erase(found);

            if (orders.empty())
                asks.erase(level);

            return true;
        }
    }

    return false;
}

bool OrderBook::replaceOrder(
    long long orderId,
    int newQuantity,
    long long newPrice
)
{
    if (newQuantity <= 0 ||
        newPrice <= 0)
    {
        if (verbose)
        {
            std::cout
                << "Replace rejected for order #"
                << orderId
                << ": invalid quantity/price\n";
        }

        return false;
    }

    auto found = orderIndex.find(orderId);

    if (found == orderIndex.end())
    {
        if (verbose)
        {
            std::cout
                << "Replace failed: order #"
                << orderId
                << " not found\n";
        }

        return false;
    }

    OrderLocation location = found->second;

    Order replacement{};
    bool orderFound = false;

    if (location.side == Side::Buy)
    {
        auto level = bids.find(location.price);

        if (level == bids.end())
            return false;

        for (const Order& order : level->second)
        {
            if (order.id == orderId)
            {
                replacement = order;
                orderFound = true;
                break;
            }
        }
    }
    else
    {
        auto level = asks.find(location.price);

        if (level == asks.end())
            return false;

        for (const Order& order : level->second)
        {
            if (order.id == orderId)
            {
                replacement = order;
                orderFound = true;
                break;
            }
        }
    }

    if (!orderFound)
        return false;

    // This returns the old reservation.
    if (!cancelOrder(orderId))
        return false;

    replacement.quantity = newQuantity;
    replacement.price = newPrice;
    replacement.type = OrderType::Limit;

    if (verbose)
    {
        std::cout
            << "Replacing order #"
            << orderId
            << ": "
            << (
                replacement.side == Side::Buy
                ? "BUY "
                : "SELL "
                )
            << newQuantity
            << ' '
            << symbol
            << " at "
            << dollars(newPrice)
            << '\n';
    }

    // Re-submitting means it loses its old
    // time priority.
    addOrder(replacement);

    return true;
}

void OrderBook::printBook() const
{
    std::cout
        << "\n--- OPEN ORDER BOOK: "
        << symbol
        << " ---\n";

    std::cout << "SELL ORDERS\n";

    for (const auto& [price, orders] : asks)
    {
        int totalQuantity = 0;

        for (const Order& order : orders)
            totalQuantity += order.quantity;

        std::cout
            << dollars(price)
            << " | "
            << totalQuantity
            << " shares\n";
    }

    std::cout
        << "-----------------------------\n"
        << "BUY ORDERS\n";

    for (const auto& [price, orders] : bids)
    {
        int totalQuantity = 0;

        for (const Order& order : orders)
            totalQuantity += order.quantity;

        std::cout
            << dollars(price)
            << " | "
            << totalQuantity
            << " shares\n";
    }
}

void OrderBook::printStatistics() const
{
    std::cout
        << "\n--- MARKET STATISTICS: "
        << symbol
        << " ---\n";

    std::cout
        << "Trades:      "
        << tradeCount
        << '\n';

    std::cout
        << "Volume:      "
        << totalVolume
        << " shares\n";

    if (tradeCount == 0)
    {
        std::cout
            << "No trades yet.\n";
        return;
    }

    std::cout
        << "Last:        "
        << dollars(lastTradePrice)
        << '\n';

    std::cout
        << "High:        "
        << dollars(highPrice)
        << '\n';

    std::cout
        << "Low:         "
        << dollars(lowPrice)
        << '\n';

    long long vwap =
        totalVolume == 0
        ? 0
        : totalValue / totalVolume;

    std::cout
        << "VWAP:        "
        << dollars(vwap)
        << '\n';

    if (!bids.empty())
    {
        std::cout
            << "Best Bid:    "
            << dollars(bids.begin()->first)
            << '\n';
    }

    if (!asks.empty())
    {
        std::cout
            << "Best Ask:    "
            << dollars(asks.begin()->first)
            << '\n';
    }

    if (!bids.empty() &&
        !asks.empty())
    {
        long long spread =
            asks.begin()->first -
            bids.begin()->first;

        std::cout
            << "Spread:      "
            << dollars(spread)
            << '\n';
    }
}

void OrderBook::cancelRemainingOrders()
{
    for (auto& [price, orders] : bids)
    {
        for (const Order& order : orders)
        {
            accounts[order.traderid].cash +=
                order.price * order.quantity;
        }
    }

    for (auto& [price, orders] : asks)
    {
        for (const Order& order : orders)
        {
            accounts[order.traderid]
                .shares[symbol] +=
                order.quantity;
        }
    }

    bids.clear();
    asks.clear();

    /*
        All resting orders are gone,
        therefore the index must also be empty.
    */
    orderIndex.clear();
}

int OrderBook::getTradeCount() const
{
    return tradeCount;
}

long long OrderBook::getTotalVolume() const
{
    return totalVolume;
}

long long OrderBook::getLastTradePrice() const
{
    return lastTradePrice;
}

int OrderBook::getOpenOrderCount() const
{
    /*
        Nice side effect of having an index:
        we no longer need to walk the entire
        order book just to count open orders.
    */
    return static_cast<int>(
        orderIndex.size()
        );
}