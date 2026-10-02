add_executable(MiniExchange
    "MiniExchange.cpp"
    "OrderBook.cpp"
    "OrderBook.h"
    "Order.h"
    "Exchange.cpp"
    "Exchange.h"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(
        TARGET MiniExchange
        PROPERTY CXX_STANDARD 20
    )
endif()