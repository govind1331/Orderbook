#include <iostream>
#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    LIMIT,
    MARKET
};

struct Order {
    uint64_t id;
    OrderSide side;
    OrderType type;
    double price;
    uint32_t quantity;
    uint32_t remaining_quantity;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    Order(uint64_t id, OrderSide side, OrderType type, double price, uint32_t quantity)
        : id(id), side(side), type(type), price(price), quantity(quantity),
          remaining_quantity(quantity), timestamp(std::chrono::high_resolution_clock::now()) {}
};

struct Trade {
    uint64_t buyer_id;
    uint64_t seller_id;
    double price;
    uint32_t quantity;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    Trade(uint64_t buyer_id, uint64_t seller_id, double price, uint32_t quantity)
        : buyer_id(buyer_id), seller_id(seller_id), price(price), quantity(quantity),
          timestamp(std::chrono::high_resolution_clock::now()) {}
};

// Comparator for buy orders (higher price has priority, then earlier timestamp)
struct BuyOrderComparator {
    bool operator()(const std::shared_ptr<Order>& a, const std::shared_ptr<Order>& b) const {
        if (a->price != b->price) {
            return a->price < b->price; // Higher price has priority (max heap)
        }
        return a->timestamp > b->timestamp; // Earlier timestamp has priority
    }
};

// Comparator for sell orders (lower price has priority, then earlier timestamp)
struct SellOrderComparator {
    bool operator()(const std::shared_ptr<Order>& a, const std::shared_ptr<Order>& b) const {
        if (a->price != b->price) {
            return a->price > b->price; // Lower price has priority (min heap)
        }
        return a->timestamp > b->timestamp; // Earlier timestamp has priority
    }
};

class OrderBook {
private:
    // Priority queues for buy and sell orders
    std::priority_queue<std::shared_ptr<Order>, std::vector<std::shared_ptr<Order>>, BuyOrderComparator> buy_orders;
    std::priority_queue<std::shared_ptr<Order>, std::vector<std::shared_ptr<Order>>, SellOrderComparator> sell_orders;
    
    // Maps for quick order lookup and cancellation
    std::map<uint64_t, std::shared_ptr<Order>> order_map;
    
    // Trade history
    std::vector<Trade> trades;
    
    uint64_t next_order_id;
    
    void clean_empty_orders() {
        // Remove completed orders from top of buy queue
        while (!buy_orders.empty() && buy_orders.top()->remaining_quantity == 0) {
            buy_orders.pop();
        }
        
        // Remove completed orders from top of sell queue
        while (!sell_orders.empty() && sell_orders.top()->remaining_quantity == 0) {
            sell_orders.pop();
        }
    }

public:
    OrderBook() : next_order_id(1) {}
    
    uint64_t add_limit_order(OrderSide side, double price, uint32_t quantity) {
        auto order = std::make_shared<Order>(next_order_id++, side, OrderType::LIMIT, price, quantity);
        order_map[order->id] = order;
        
        if (side == OrderSide::BUY) {
            // Try to match with existing sell orders
            match_buy_order(order);
            if (order->remaining_quantity > 0) {
                buy_orders.push(order);
            }
        } else {
            // Try to match with existing buy orders
            match_sell_order(order);
            if (order->remaining_quantity > 0) {
                sell_orders.push(order);
            }
        }
        
        return order->id;
    }
    
    std::vector<Trade> add_market_order(OrderSide side, uint32_t quantity) {
        auto order = std::make_shared<Order>(next_order_id++, side, OrderType::MARKET, 0.0, quantity);
        std::vector<Trade> order_trades;
        
        if (side == OrderSide::BUY) {
            // Market buy: match against sell orders at any price
            while (order->remaining_quantity > 0 && !sell_orders.empty()) {
                clean_empty_orders();
                if (sell_orders.empty()) break;
                
                auto best_sell = sell_orders.top();
                if (best_sell->remaining_quantity == 0) continue;
                
                uint32_t trade_quantity = std::min(order->remaining_quantity, best_sell->remaining_quantity);
                
                trades.emplace_back(order->id, best_sell->id, best_sell->price, trade_quantity);
                order_trades.push_back(trades.back());
                
                order->remaining_quantity -= trade_quantity;
                best_sell->remaining_quantity -= trade_quantity;
                
                if (best_sell->remaining_quantity == 0) {
                    order_map.erase(best_sell->id);
                }
            }
        } else {
            // Market sell: match against buy orders at any price
            while (order->remaining_quantity > 0 && !buy_orders.empty()) {
                clean_empty_orders();
                if (buy_orders.empty()) break;
                
                auto best_buy = buy_orders.top();
                if (best_buy->remaining_quantity == 0) continue;
                
                uint32_t trade_quantity = std::min(order->remaining_quantity, best_buy->remaining_quantity);
                
                trades.emplace_back(best_buy->id, order->id, best_buy->price, trade_quantity);
                order_trades.push_back(trades.back());
                
                order->remaining_quantity -= trade_quantity;
                best_buy->remaining_quantity -= trade_quantity;
                
                if (best_buy->remaining_quantity == 0) {
                    order_map.erase(best_buy->id);
                }
            }
        }
        
        return order_trades;
    }
    
    bool cancel_order(uint64_t order_id) {
        auto it = order_map.find(order_id);
        if (it != order_map.end()) {
            it->second->remaining_quantity = 0; // Mark as cancelled
            order_map.erase(it);
            return true;
        }
        return false;
    }
    
    void match_buy_order(std::shared_ptr<Order> buy_order) {
        while (buy_order->remaining_quantity > 0 && !sell_orders.empty()) {
            clean_empty_orders();
            if (sell_orders.empty()) break;
            
            auto best_sell = sell_orders.top();
            if (best_sell->remaining_quantity == 0) continue;
            
            // Check if prices cross
            if (buy_order->price < best_sell->price) {
                break;
            }
            
            uint32_t trade_quantity = std::min(buy_order->remaining_quantity, best_sell->remaining_quantity);
            
            trades.emplace_back(buy_order->id, best_sell->id, best_sell->price, trade_quantity);
            
            buy_order->remaining_quantity -= trade_quantity;
            best_sell->remaining_quantity -= trade_quantity;
            
            if (best_sell->remaining_quantity == 0) {
                order_map.erase(best_sell->id);
            }
        }
    }
    
    void match_sell_order(std::shared_ptr<Order> sell_order) {
        while (sell_order->remaining_quantity > 0 && !buy_orders.empty()) {
            clean_empty_orders();
            if (buy_orders.empty()) break;
            
            auto best_buy = buy_orders.top();
            if (best_buy->remaining_quantity == 0) continue;
            
            // Check if prices cross
            if (sell_order->price > best_buy->price) {
                break;
            }
            
            uint32_t trade_quantity = std::min(sell_order->remaining_quantity, best_buy->remaining_quantity);
            
            trades.emplace_back(best_buy->id, sell_order->id, best_buy->price, trade_quantity);
            
            sell_order->remaining_quantity -= trade_quantity;
            best_buy->remaining_quantity -= trade_quantity;
            
            if (best_buy->remaining_quantity == 0) {
                order_map.erase(best_buy->id);
            }
        }
    }
    
    double get_best_bid() const {
        auto temp_buy_orders = buy_orders;
        while (!temp_buy_orders.empty()) {
            auto order = temp_buy_orders.top();
            if (order->remaining_quantity > 0) {
                return order->price;
            }
            temp_buy_orders.pop();
        }
        return 0.0; // No bids
    }
    
    double get_best_ask() const {
        auto temp_sell_orders = sell_orders;
        while (!temp_sell_orders.empty()) {
            auto order = temp_sell_orders.top();
            if (order->remaining_quantity > 0) {
                return order->price;
            }
            temp_sell_orders.pop();
        }
        return 0.0; // No asks
    }
    
    void print_book(int depth = 5) const {
        std::cout << "\n=== ORDER BOOK ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        
        // Get sell orders (asks) - reverse order for display
        std::vector<std::pair<double, uint32_t>> asks;
        auto temp_sell = sell_orders;
        std::map<double, uint32_t> sell_levels;
        
        while (!temp_sell.empty() && asks.size() < depth) {
            auto order = temp_sell.top();
            temp_sell.pop();
            if (order->remaining_quantity > 0) {
                sell_levels[order->price] += order->remaining_quantity;
            }
        }
        
        for (auto it = sell_levels.rbegin(); it != sell_levels.rend(); ++it) {
            asks.push_back({it->first, it->second});
        }
        
        // Print asks (highest first)
        for (auto& ask : asks) {
            std::cout << "ASK: " << std::setw(8) << ask.first 
                      << " | " << std::setw(6) << ask.second << std::endl;
        }
        
        std::cout << "     --------+--------" << std::endl;
        
        // Get buy orders (bids)
        auto temp_buy = buy_orders;
        std::map<double, uint32_t> buy_levels;
        int count = 0;
        
        while (!temp_buy.empty() && count < depth) {
            auto order = temp_buy.top();
            temp_buy.pop();
            if (order->remaining_quantity > 0) {
                if (buy_levels.find(order->price) == buy_levels.end()) {
                    count++;
                }
                buy_levels[order->price] += order->remaining_quantity;
            }
        }
        
        // Print bids (highest first)
        for (auto it = buy_levels.rbegin(); it != buy_levels.rend(); ++it) {
            std::cout << "BID: " << std::setw(8) << it->first 
                      << " | " << std::setw(6) << it->second << std::endl;
        }
        
        std::cout << "==================" << std::endl;
    }
    
    void print_trades(int count = 10) const {
        std::cout << "\n=== RECENT TRADES ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        
        int start = std::max(0, (int)trades.size() - count);
        for (int i = start; i < trades.size(); ++i) {
            const auto& trade = trades[i];
            std::cout << "Trade: Buyer=" << trade.buyer_id 
                      << " Seller=" << trade.seller_id
                      << " Price=" << trade.price
                      << " Qty=" << trade.quantity << std::endl;
        }
        std::cout << "====================" << std::endl;
    }
    
    size_t get_trade_count() const { return trades.size(); }
};

// Example usage and testing
int main() {
    OrderBook book;
    
    std::cout << "=== Market Order Book Demo ===" << std::endl;
    
    // Add some limit orders
    std::cout << "\nAdding limit orders..." << std::endl;
    book.add_limit_order(OrderSide::BUY, 100.50, 100);   // Buy 100 @ 100.50
    book.add_limit_order(OrderSide::BUY, 100.25, 200);   // Buy 200 @ 100.25
    book.add_limit_order(OrderSide::BUY, 100.75, 50);    // Buy 50 @ 100.75
    
    book.add_limit_order(OrderSide::SELL, 101.00, 150);  // Sell 150 @ 101.00
    book.add_limit_order(OrderSide::SELL, 101.25, 100);  // Sell 100 @ 101.25
    book.add_limit_order(OrderSide::SELL, 100.90, 75);   // Sell 75 @ 100.90
    
    book.print_book();
    
    // Add a buy order that crosses the spread
    std::cout << "\nAdding aggressive buy order (101.10 for 200 shares)..." << std::endl;
    book.add_limit_order(OrderSide::BUY, 101.10, 200);
    
    book.print_book();
    book.print_trades();
    
    // Add a market sell order
    std::cout << "\nExecuting market sell for 120 shares..." << std::endl;
    auto market_trades = book.add_market_order(OrderSide::SELL, 120);
    std::cout << "Market order generated " << market_trades.size() << " trades" << std::endl;
    
    book.print_book();
    book.print_trades();
    
    // Test order cancellation
    std::cout << "\nAdding order to cancel..." << std::endl;
    uint64_t cancel_id = book.add_limit_order(OrderSide::BUY, 99.50, 300);
    book.print_book();
    
    std::cout << "\nCancelling order " << cancel_id << "..." << std::endl;
    book.cancel_order(cancel_id);
    book.print_book();
    
    std::cout << "\nBest Bid: $" << book.get_best_bid() << std::endl;
    std::cout << "Best Ask: $" << book.get_best_ask() << std::endl;
    std::cout << "Total Trades: " << book.get_trade_count() << std::endl;
    
    return 0;
}