#include "Exchange.h"

#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

void printAccounts(
    const std::vector<Account>& accounts
)
{
    std::cout << "\n--- ACCOUNTS ---\n";

    for (const Account& account : accounts)
    {
        std::cout
            << account.name
            << " | Cash: "
            << dollars(account.cash);

        for (const auto& [symbol, shares] :
            account.shares)
        {
            std::cout
                << " | "
                << symbol
                << ": "
                << shares;
        }

        std::cout << '\n';
    }
}

void runDemo()
{
    std::cout
        << "=================================\n"
        << "       MINI EXCHANGE DEMO\n"
        << "=================================\n";

    std::vector<Account> accounts{
        {
            "Alex",
            100000000,
            {
                {"AAPL", 10000},
                {"MSFT", 10000},
                {"NVDA", 10000}
            }
        },
        {
            "Blair",
            100000000,
            {
                {"AAPL", 10000},
                {"MSFT", 10000},
                {"NVDA", 10000}
            }
        },
        {
            "Casey",
            100000000,
            {
                {"AAPL", 10000},
                {"MSFT", 10000},
                {"NVDA", 10000}
            }
        },
        {
            "Drew",
            100000000,
            {
                {"AAPL", 10000},
                {"MSFT", 10000},
                {"NVDA", 10000}
            }
        }
    };

    Exchange exchange(accounts);

    exchange.addSymbol("AAPL");
    exchange.addSymbol("MSFT");
    exchange.addSymbol("NVDA");

    printAccounts(accounts);

    std::cout
        << "\n--- LIMIT ORDER TEST ---\n";

    // Drew sells 100 AAPL at $200.
    exchange.addOrder({
        1,
        "AAPL",
        Side::Sell,
        OrderType::Limit,
        100,
        20000,
        3
        });

    // Alex buys 40 at $201.
    // This should execute 40 at the resting
    // seller's $200 price.
    exchange.addOrder({
        2,
        "AAPL",
        Side::Buy,
        OrderType::Limit,
        40,
        20100,
        0
        });

    std::cout
        << "\n--- PARTIAL FILL TEST ---\n";

    // Blair buys another 25.
    exchange.addOrder({
        3,
        "AAPL",
        Side::Buy,
        OrderType::Limit,
        25,
        20000,
        1
        });

    OrderBook* apple =
        exchange.getBook("AAPL");

    if (apple)
    {
        apple->printBook();
        apple->printStatistics();
    }

    std::cout
        << "\n--- MULTIPLE PRICE LEVEL TEST ---\n";

    exchange.addOrder({
        4,
        "AAPL",
        Side::Sell,
        OrderType::Limit,
        20,
        20100,
        3
        });

    exchange.addOrder({
        5,
        "AAPL",
        Side::Sell,
        OrderType::Limit,
        20,
        20200,
        3
        });

    // Casey market buys 60 shares.
    // It should consume the cheapest
    // available asks first.
    exchange.addOrder({
        6,
        "AAPL",
        Side::Buy,
        OrderType::Market,
        60,
        0,
        2
        });

    if (apple)
    {
        apple->printBook();
        apple->printStatistics();
    }

    std::cout
        << "\n--- CANCELLATION TEST ---\n";

    // Put an order far away so it rests.
    exchange.addOrder({
        7,
        "AAPL",
        Side::Buy,
        OrderType::Limit,
        50,
        19000,
        0
        });

    if (apple)
        apple->printBook();

    std::cout
        << "\nCancelling order #7...\n";

    exchange.cancelOrder("AAPL", 7);

    if (apple)
        apple->printBook();

    std::cout
        << "\n--- ORDER REPLACEMENT TEST ---\n";

    // Create an order that won't immediately match.
    exchange.addOrder({
        10,
        "AAPL",
        Side::Buy,
        OrderType::Limit,
        100,
        19500,
        0
        });

    if (apple)
        apple->printBook();

    std::cout
        << "\nReplacing order #10...\n";

    // Change:
    // BUY 100 @ $195
    //
    // into:
    //
    // BUY 150 @ $199
    exchange.replaceOrder(
        "AAPL",
        10,
        150,
        19900
    );

    if (apple)
        apple->printBook();

    std::cout
        << "\n--- MULTI-SYMBOL TEST ---\n";

    exchange.addOrder({
        8,
        "MSFT",
        Side::Sell,
        OrderType::Limit,
        50,
        45000,
        3
        });

    exchange.addOrder({
        9,
        "MSFT",
        Side::Buy,
        OrderType::Market,
        20,
        0,
        0
        });

    OrderBook* microsoft =
        exchange.getBook("MSFT");

    if (microsoft)
    {
        microsoft->printBook();
        microsoft->printStatistics();
    }

    printAccounts(accounts);

    exchange.cancelAllRemainingOrders();

    std::cout
        << "\n--- AFTER ALL OPEN ORDERS CANCELLED ---\n";

    printAccounts(accounts);
}



void runBenchmark(int orderCount)
{
    std::cout
        << "\n=================================\n"
        << "          BENCHMARK\n"
        << "=================================\n";

    std::cout
        << "Generating "
        << orderCount
        << " orders...\n";

    std::vector<Account> accounts;

    constexpr int traderCount = 100;

    for (int i = 0; i < traderCount; ++i)
    {
        accounts.push_back({
            "Trader" + std::to_string(i),
            100000000000LL,
            {
                {"AAPL", 10000000}
            }
            });
    }

    Exchange exchange(accounts);
    exchange.addSymbol("AAPL");

    OrderBook* book =
        exchange.getBook("AAPL");

    if (!book)
        return;

    // IMPORTANT:
    // Turn off console output during benchmark.
    book->setVerbose(false);

    std::mt19937 rng(12345);

    std::bernoulli_distribution buySell(0.5);

    std::uniform_int_distribution<int>
        quantityDist(1, 100);

    std::uniform_int_distribution<int>
        priceDist(19500, 20500);

    std::uniform_int_distribution<int>
        traderDist(0, traderCount - 1);

    auto start =
        std::chrono::steady_clock::now();

    for (int i = 1; i <= orderCount; ++i)
    {
        Side side =
            buySell(rng)
            ? Side::Buy
            : Side::Sell;

        int trader = traderDist(rng);

        exchange.addOrder({
            i,
            "AAPL",
            side,
            OrderType::Limit,
            quantityDist(rng),
            priceDist(rng),
            trader
            });
    }

    auto end =
        std::chrono::steady_clock::now();

    auto milliseconds =
        std::chrono::duration_cast<
        std::chrono::milliseconds
        >(end - start);

    std::cout
        << "Completed in: "
        << milliseconds.count()
        << " ms\n";

    std::cout
        << "Trades:       "
        << book->getTradeCount()
        << '\n';

    std::cout
        << "Volume:       "
        << book->getTotalVolume()
        << " shares\n";

    std::cout
        << "Open orders:  "
        << book->getOpenOrderCount()
        << '\n';

    if (milliseconds.count() > 0)
    {
        double ordersPerSecond =
            static_cast<double>(orderCount) /
            (
                static_cast<double>(
                    milliseconds.count()
                    ) / 1000.0
                );

        std::cout
            << "Throughput:   "
            << static_cast<long long>(
                ordersPerSecond
                )
            << " orders/sec\n";
    }

    exchange.cancelAllRemainingOrders();
}

int main()
{
    runDemo();
    std::cout
        << "\n\nPress ENTER to start benchmark...";
    std::cin.get();

    runBenchmark(1000);
    runBenchmark(10000);
    runBenchmark(100000);
    runBenchmark(1000000);

    std::cout
        << "\nPress ENTER to exit...";
    std::cin.get();

    return 0;
}