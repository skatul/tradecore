#include "messaging/protocol.hpp"

namespace tradecore::messaging {

std::string serialize(const fix::FixMessage& msg) {
    return msg.SerializeAsString();
}

fix::FixMessage deserialize(const std::string& data) {
    fix::FixMessage msg;
    msg.ParseFromString(data);
    return msg;
}

fix::FixMessage deserialize(const void* data, size_t size) {
    fix::FixMessage msg;
    msg.ParseFromArray(data, static_cast<int>(size));
    return msg;
}

fix::FixMessage make_execution_report_new(
    const fix::FixMessage& request,
    const std::string& order_id) {

    const auto& nos = request.new_order_single();

    fix::FixMessage msg;
    msg.set_sender_comp_id("TRADECORE");
    msg.set_target_comp_id(request.sender_comp_id());
    msg.set_msg_seq_num(generate_uuid());
    msg.set_sending_time(current_timestamp());

    auto* er = msg.mutable_execution_report();
    er->set_order_id(order_id);
    er->set_cl_ord_id(nos.cl_ord_id());
    er->set_exec_id(generate_uuid());
    er->set_exec_type(fix::EXEC_TYPE_NEW);
    er->set_ord_status(fix::ORD_STATUS_NEW);
    *er->mutable_instrument() = nos.instrument();
    er->set_side(nos.side());
    er->set_order_qty(nos.order_qty());
    er->set_leaves_qty(nos.order_qty());
    er->set_cum_qty(0.0);
    er->set_avg_px(0.0);
    er->set_transact_time(current_timestamp());

    return msg;
}

fix::FixMessage make_execution_report_fill(
    const fix::FixMessage& request,
    const std::string& order_id,
    const std::string& exec_id,
    double last_px,
    double last_qty,
    double leaves_qty,
    double cum_qty,
    double commission) {

    const auto& nos = request.new_order_single();

    fix::FixMessage msg;
    msg.set_sender_comp_id("TRADECORE");
    msg.set_target_comp_id(request.sender_comp_id());
    msg.set_msg_seq_num(generate_uuid());
    msg.set_sending_time(current_timestamp());

    auto* er = msg.mutable_execution_report();
    er->set_order_id(order_id);
    er->set_cl_ord_id(nos.cl_ord_id());
    er->set_exec_id(exec_id);
    er->set_exec_type(leaves_qty == 0.0 ? fix::EXEC_TYPE_FILL : fix::EXEC_TYPE_PARTIAL_FILL);
    er->set_ord_status(leaves_qty == 0.0 ? fix::ORD_STATUS_FILLED : fix::ORD_STATUS_PARTIALLY_FILLED);
    *er->mutable_instrument() = nos.instrument();
    er->set_side(nos.side());
    er->set_order_qty(nos.order_qty());
    er->set_last_px(last_px);
    er->set_last_qty(last_qty);
    er->set_leaves_qty(leaves_qty);
    er->set_cum_qty(cum_qty);
    er->set_avg_px(last_px);
    er->set_commission(commission);
    er->set_transact_time(current_timestamp());

    return msg;
}

fix::FixMessage make_reject(
    const fix::FixMessage& request,
    const std::string& reason) {

    fix::FixMessage msg;
    msg.set_sender_comp_id("TRADECORE");
    msg.set_target_comp_id(request.sender_comp_id());
    msg.set_msg_seq_num(generate_uuid());
    msg.set_sending_time(current_timestamp());

    auto* rej = msg.mutable_reject();
    rej->set_ref_msg_seq_num(request.msg_seq_num());
    rej->set_text(reason);

    return msg;
}

fix::FixMessage make_heartbeat_response(const fix::FixMessage& request) {
    fix::FixMessage msg;
    msg.set_sender_comp_id("TRADECORE");
    msg.set_target_comp_id(request.sender_comp_id());
    msg.set_msg_seq_num(generate_uuid());
    msg.set_sending_time(current_timestamp());

    auto* hb = msg.mutable_heartbeat();
    if (request.has_heartbeat()) {
        hb->set_test_req_id(request.heartbeat().test_req_id());
    }

    return msg;
}

fix::FixMessage make_position_report(
    const fix::FixMessage& request,
    const std::string& rpt_id) {

    fix::FixMessage msg;
    msg.set_sender_comp_id("TRADECORE");
    msg.set_target_comp_id(request.sender_comp_id());
    msg.set_msg_seq_num(generate_uuid());
    msg.set_sending_time(current_timestamp());

    auto* pr = msg.mutable_position_report();
    if (request.has_position_request()) {
        pr->set_pos_req_id(request.position_request().pos_req_id());
    }
    pr->set_pos_rpt_id(rpt_id);

    return msg;
}

}  // namespace tradecore::messaging
