#pragma once

#include <cstdint>
#include <optional>

#include <mavconn/mavlink_dialect.hpp>
#include <mavros_msgs/mavlink_convert.hpp>
#include <mavros_msgs/msg/mavlink.hpp>

namespace mowgli_mavros_bridge
{

inline std::optional<uint8_t> decode_button_change(const mavros_msgs::msg::Mavlink & message)
{
  using ButtonChange = mavlink::common::msg::BUTTON_CHANGE;
  if (message.framing_status != mavros_msgs::msg::Mavlink::FRAMING_OK ||
    message.msgid != ButtonChange::MSG_ID)
  {
    return std::nullopt;
  }

  mavlink::mavlink_message_t mavlink_message{};
  if (!mavros_msgs::mavlink::convert(message, mavlink_message)) {
    return std::nullopt;
  }

  mavlink::MsgMap map(&mavlink_message);
  ButtonChange button_change;
  button_change.deserialize(map);
  return button_change.state;
}

}  // namespace mowgli_mavros_bridge
