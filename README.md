# Order Book Engine

A high-performance, exchange-style order book implementation in C++ featuring real-time order matching, priority-based execution, and comprehensive trade tracking.

## Overview

This project implements a fully functional limit order book (LOB) system similar to those used in financial exchanges. The order book efficiently matches buy and sell orders using price-time priority, supports both limit and market orders, and maintains a complete trade history.

## Features

- **Limit Orders**: Place buy/sell orders at specific price levels
- **Market Orders**: Execute immediately at the best available prices
- **Price-Time Priority**: Orders matched based on best price, then earliest timestamp
- **Order Cancellation**: Cancel pending orders before execution
- **Real-time Matching Engine**: Automatic order matching when prices cross
- **Trade History**: Complete audit trail of all executed trades
- **Order Book Display**: Visual representation of current bid/ask levels
- **Efficient Data Structures**: Priority queues for O(log n) insertions and O(1) best price lookups

## Architecture

### Core Components

**Order Structure**
- Unique order ID
- Side (BUY/SELL) and type (LIMIT/MARKET)
- Price and quantity tracking
- High-resolution timestamps for priority

**OrderBook Class**
- Separate priority queues for buy and sell orders
- Fast order lookup via hash map
- Automatic order matching logic
- Trade execution and recording

**Matching Algorithm**
- Buy orders prioritized by highest price, then earliest time
- Sell orders prioritized by lowest price, then earliest time
- Partial order fills supported
- Executed at the resting order's price (maker price)

## Building and Running

### Prerequisites
- C++11 or higher
- Standard C++ compiler (g++, clang++, MSVC)

### Compilation

```bash
g++ -std=c++11 -o orderbook orderbook.cpp
```

### Execution

```bash
./orderbook
```

## Usage Example

```cpp
OrderBook book;

// Add limit orders
book.add_limit_order(OrderSide::BUY, 100.50, 100);   // Buy 100 @ $100.50
book.add_limit_order(OrderSide::SELL, 101.00, 150);  // Sell 150 @ $101.00

// Display current order book
book.print_book();

// Execute market order
auto trades = book.add_market_order(OrderSide::BUY, 50);

// Cancel an order
uint64_t order_id = book.add_limit_order(OrderSide::BUY, 99.00, 200);
book.cancel_order(order_id);

// Get best prices
double best_bid = book.get_best_bid();
double best_ask = book.get_best_ask();
```

## Order Matching Rules

1. **Price Priority**: Buy orders with higher prices and sell orders with lower prices execute first
2. **Time Priority**: Among orders at the same price level, earlier orders execute first
3. **Pro-Rata Not Implemented**: Orders filled completely before moving to next order
4. **Maker Price**: Trades execute at the price of the resting (passive) order

## Sample Output

```
=== ORDER BOOK ===
ASK:   101.25 |    100
ASK:   101.00 |    150
ASK:   100.90 |     75
     --------+--------
BID:   100.75 |     50
BID:   100.50 |    100
BID:   100.25 |    200
==================

=== RECENT TRADES ===
Trade: Buyer=7 Seller=6 Price=100.90 Qty=75
Trade: Buyer=7 Seller=4 Price=101.00 Qty=125
====================
```

## Performance Characteristics

- **Add Order**: O(log n) - priority queue insertion
- **Match Order**: O(m log n) - where m is number of matches
- **Cancel Order**: O(log n) - hash map lookup + lazy deletion
- **Get Best Bid/Ask**: O(1) amortized - peek top of priority queue
- **Space Complexity**: O(n) - where n is number of active orders

## Future Enhancements

- [ ] Order modification without cancellation
- [ ] Stop and stop-limit orders
- [ ] Iceberg (hidden) orders
- [ ] Order book snapshots and replay
- [ ] Multi-threaded support with lock-free structures
- [ ] Market depth aggregation
- [ ] REST/WebSocket API integration
- [ ] Persistence layer for order recovery

## Technical Details

### Data Structures
- **Priority Queues**: STL `priority_queue` with custom comparators
- **Order Lookup**: STL `map` for O(log n) cancellation
- **Trade Storage**: STL `vector` for sequential trade history

### Memory Management
- Smart pointers (`shared_ptr`) for automatic memory management
- Lazy deletion strategy to avoid priority queue modifications

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Author

Built as a demonstration of exchange-style order matching systems and efficient C++ data structure usage.