// Passive, bounded startup probe. Never send commands or change FCU state.
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

#include <mavconn/interface.hpp>

namespace
{
int integer_argument(const char * value, int minimum, int maximum)
{
  std::size_t consumed = 0;
  const std::string text(value);
  const int number = std::stoi(text, &consumed);
  if (consumed != text.size() || number < minimum || number > maximum) {
    throw std::runtime_error("invalid detection argument");
  }
  return number;
}
}  // namespace

int main(int argc, char ** argv)
{
  if (argc != 4 && argc != 5) {
    std::cerr << "usage: detect_mavros_firmware FCU_URL TARGET_SYSTEM TARGET_COMPONENT [TIMEOUT_MS]\n";
    return 1;
  }
  try {
    const int system = integer_argument(argv[2], 1, 255);
    const int component = integer_argument(argv[3], 1, 255);
    const int timeout_ms = argc == 5 ? integer_argument(argv[4], 1, 10000) : 10000;
    std::mutex mutex;
    std::condition_variable ready;
    bool received = false;
    bool closed = false;
    std::string firmware;
    auto connection = mavconn::MAVConnInterface::open_url(
      argv[1], 255, mavconn::MAV_COMP_ID_UDP_BRIDGE,
      [&](const mavlink::mavlink_message_t * message, mavconn::Framing framing) {
        if (framing != mavconn::Framing::ok || message->sysid != system ||
          message->compid != component ||
          message->msgid != mavlink::minimal::msg::HEARTBEAT::MSG_ID || message->len < 9)
        {
          return;
        }
        mavlink::minimal::msg::HEARTBEAT heartbeat;
        mavlink::MsgMap map(message);
        heartbeat.deserialize(map);
        // An invalid autopilot field is used by GCS/peripheral heartbeats.
        if (heartbeat.autopilot == static_cast<uint8_t>(
            mavlink::minimal::MAV_AUTOPILOT::INVALID))
        {
          return;
        }
        std::lock_guard<std::mutex> lock(mutex);
        if (received) {
          return;
        }
        switch (static_cast<mavlink::minimal::MAV_AUTOPILOT>(heartbeat.autopilot)) {
          case mavlink::minimal::MAV_AUTOPILOT::ARDUPILOTMEGA:
            firmware = "ardupilot";
            break;
          case mavlink::minimal::MAV_AUTOPILOT::PX4:
            firmware = "px4";
            break;
          default:
            firmware = "unknown";
            break;
        }
        received = true;
        ready.notify_one();
      },
      [&]() {
        std::lock_guard<std::mutex> lock(mutex);
        closed = true;
        ready.notify_one();
      });
    {
      std::unique_lock<std::mutex> lock(mutex);
      ready.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&]() {
        return received || closed;
      });
    }
    // Stop callbacks and release the serial device/listening port before MAVROS.
    connection->close();
    connection.reset();
    std::lock_guard<std::mutex> lock(mutex);
    if (!received) {
      throw std::runtime_error("no target FCU heartbeat received before timeout or connection closure");
    }
    if (firmware == "unknown") {
      throw std::runtime_error("target FCU firmware cannot be identified as ArduPilot or PX4");
    }
    std::cout << firmware << '\n';
    return 0;
  } catch (const std::exception & error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
