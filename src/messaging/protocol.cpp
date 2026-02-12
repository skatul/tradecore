#include "messaging/protocol.hpp"

namespace tradecore::messaging {

Message make_order_ack(const Message& request, const std::string& order_id) {
    json payload;
    payload["cl_ord_id"] = request.payload.at("cl_ord_id");
    payload["order_id"] = order_id;
    payload["status"] = "accepted";
    return Message::make_response(request, "order_ack", payload);
}

Message make_fill(const Message& request, const std::string& order_id,
                  const std::string& fill_id, double fill_price,
                  double fill_qty, double remaining_qty, double commission) {
    json payload;
    payload["cl_ord_id"] = request.payload.at("cl_ord_id");
    payload["order_id"] = order_id;
    payload["fill_id"] = fill_id;
    payload["fill_price"] = fill_price;
    payload["fill_quantity"] = fill_qty;
    payload["remaining_quantity"] = remaining_qty;
    payload["status"] = (remaining_qty == 0.0) ? "filled" : "partially_filled";
    payload["commission"] = commission;
    return Message::make_response(request, "fill", payload);
}

Message make_reject(const Message& request, const std::string& reason,
                    const std::string& detail) {
    json payload;
    if (request.payload.contains("cl_ord_id")) {
        payload["cl_ord_id"] = request.payload["cl_ord_id"];
    }
    payload["order_id"] = nullptr;
    payload["reason"] = reason;
    payload["detail"] = detail;
    return Message::make_response(request, "reject", payload);
}

}  // namespace tradecore::messaging
