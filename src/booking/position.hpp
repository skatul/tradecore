#pragma once

#include <string>

namespace tradecore::booking {

struct Position {
    std::string symbol;
    double quantity = 0.0;      // positive = long, negative = short
    double avg_price = 0.0;
    double realized_pnl = 0.0;
    double cost_basis = 0.0;

    void apply_fill(const std::string& side, double fill_qty, double fill_price) {
        if (side == "buy") {
            if (quantity >= 0) {
                // Adding to long
                cost_basis += fill_qty * fill_price;
                quantity += fill_qty;
                avg_price = (quantity != 0.0) ? cost_basis / quantity : 0.0;
            } else {
                // Closing short
                double pnl = fill_qty * (avg_price - fill_price);
                realized_pnl += pnl;
                quantity += fill_qty;
                if (quantity > 0) {
                    avg_price = fill_price;
                    cost_basis = quantity * fill_price;
                } else if (quantity == 0) {
                    avg_price = 0.0;
                    cost_basis = 0.0;
                } else {
                    cost_basis = -quantity * avg_price;
                }
            }
        } else {  // sell
            if (quantity > 0) {
                // Closing long
                double pnl = fill_qty * (fill_price - avg_price);
                realized_pnl += pnl;
                quantity -= fill_qty;
                if (quantity < 0) {
                    avg_price = fill_price;
                    cost_basis = -quantity * fill_price;
                } else if (quantity == 0) {
                    avg_price = 0.0;
                    cost_basis = 0.0;
                } else {
                    cost_basis = quantity * avg_price;
                }
            } else {
                // Adding to short
                cost_basis += fill_qty * fill_price;
                quantity -= fill_qty;
                avg_price = (quantity != 0.0) ? cost_basis / -quantity : 0.0;
            }
        }
    }
};

}  // namespace tradecore::booking
