#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace mowgli_mavros_bridge
{

// Caller supplies monotonic milliseconds. Observation identity is independent of
// delivery liveness; replaying a stamp/counter never advances the stop proof.
class BladeControl
{
public:
  enum class Direction
  {
    Off,
    Forward,
    Reverse,
    Unknown
  };
  enum class State
  {
    OFF,
    FWD,
    REV,
    TRANSITION_TO_OFF,
    WAIT_STOP_BEFORE_REVERSE,
    FAILED
  };
  enum class Result
  {
    Rejected,
    Pending,
    Confirmed
  };
  struct Config
  {
    int channel{3}, neutral{1500}, forward{1450}, reverse{1550};
    int64_t ack_timeout_ms{3000}, stop_timeout_ms{15000}, freshness_ms{1000}, coast_ms{1000};
  };
  struct Command
  {
    uint64_t token;
    int channel, pwm;
    Direction direction;
  };
  BladeControl() : BladeControl(Config{})
  {
  }
  explicit BladeControl(Config config) : config_(config)
  {
    if (config.channel < 1 || config.channel > 32 || config.neutral < 1000 ||
        config.neutral > 2000 || config.forward < 1000 || config.forward > 2000 ||
        config.reverse < 1000 || config.reverse > 2000 ||
        !((config.forward < config.neutral && config.reverse > config.neutral) ||
          (config.reverse < config.neutral && config.forward > config.neutral)) ||
        config.ack_timeout_ms <= 0 || config.stop_timeout_ms <= config.coast_ms ||
        config.freshness_ms <= 0 || config.coast_ms < 1000)
    {
      throw std::invalid_argument("Invalid blade PWM/timing configuration");
    }
  }

  void connection(bool connected, int64_t now)
  {
    if (connected != connected_)
    {
      connected_ = connected;
      sample_stamp_ = 0;
      sample_source_ = -1;
      sample_receipt_ = -1;
      force_off(now);
    }
  }
  void permission(bool allowed, int64_t now)
  {
    if (allowed_ && !allowed)
      force_off(now);
    allowed_ = allowed;
  }
  Result request(Direction direction, int64_t now)
  {
    if (!connected_ || direction == Direction::Unknown ||
        (direction != Direction::Off && (!allowed_ || fault_ || sample_receipt_ < 0 ||
                                         now - sample_receipt_ > config_.freshness_ms)))
      return Result::Rejected;
    if (fault_)
      return Result::Rejected;
    if (direction == requested_)
      return satisfied() ? Result::Confirmed : Result::Pending;
    ++revision_;
    requested_ = direction;
    if (direction == Direction::Off)
    {
      // A neutral must supersede an ON already handed to the transport.
      force_off(now);
    }
    else if (confirmed_ != Direction::Off || in_flight_ || queued_neutral_)
    {
      if (in_flight_ && in_flight_->direction != Direction::Off)
      {
        force_off(now);
        requested_ = direction;
      }
      queued_neutral_ = true;
      state_ = State::TRANSITION_TO_OFF;
    }
    else
    {
      state_ = State::WAIT_STOP_BEFORE_REVERSE;
      wait_since_ = now;
    }
    return Result::Pending;
  }
  void force_off(int64_t now)
  {
    ++revision_;
    requested_ = Direction::Off;
    confirmed_ = Direction::Unknown;
    // Fence old replies without claiming the transport can cancel an emitted PWM.
    in_flight_.reset();
    ++token_;
    queued_neutral_ = true;
    fault_ = false;
    stop_since_ = -1;
    stop_samples_ = 0;
    neutral_since_ = now;
    state_ = State::TRANSITION_TO_OFF;
  }
  void service_timeout(int64_t now)
  {
    force_off(now);
    fault_ = true;
    state_ = State::FAILED;
  }
  void sample(int source,
              int64_t stamp,
              uint32_t count,
              bool count_valid,
              bool valid,
              int32_t rpm,
              int64_t now)
  {
    if (!valid || (source != 1 && source != 2) || (source == 2 && !count_valid) || stamp <= 0 ||
        stamp <= sample_stamp_ ||
        (sample_source_ == source && source == 2 && (!count_valid || count == sample_count_)))
      return;
    if (sample_source_ != source)
    {
      stop_since_ = -1;
      stop_samples_ = 0;
    }
    sample_source_ = source;
    sample_stamp_ = stamp;
    sample_count_ = count;
    if (sample_receipt_ >= 0 && now - sample_receipt_ > config_.freshness_ms)
    {
      stop_since_ = -1;
      stop_samples_ = 0;
    }
    sample_receipt_ = now;
    sample_zero_ = rpm == 0;
    if (sample_zero_ && now >= neutral_since_ && confirmed_ == Direction::Off)
    {
      if (stop_since_ < 0)
        stop_since_ = now;
      ++stop_samples_;
    }
    else
    {
      stop_since_ = -1;
      stop_samples_ = 0;
    }
  }
  bool stopped(int64_t now) const
  {
    return sample_zero_ && sample_receipt_ >= 0 && now >= sample_receipt_ &&
           now - sample_receipt_ <= config_.freshness_ms && stop_since_ >= 0 &&
           now - stop_since_ >= config_.coast_ms && stop_samples_ >= 5;
  }
  void tick(int64_t now)
  {
    if (in_flight_ && now - sent_at_ >= config_.ack_timeout_ms)
    {
      const bool was_on = in_flight_->direction != Direction::Off;
      in_flight_.reset();
      fail(was_on);
    }
    if (requested_ != Direction::Off &&
        (sample_receipt_ < 0 || now - sample_receipt_ > config_.freshness_ms))
      fail(true);
    if (state_ == State::WAIT_STOP_BEFORE_REVERSE && !in_flight_ &&
        now - wait_since_ >= config_.stop_timeout_ms)
      fail(false);
  }
  std::optional<Command> next(int64_t now)
  {
    tick(now);
    if (in_flight_)
      return std::nullopt;
    Direction target = Direction::Unknown;
    if (queued_neutral_)
    {
      queued_neutral_ = false;
      target = Direction::Off;
    }
    else if (!fault_ && connected_ && allowed_ && requested_ != Direction::Off &&
             confirmed_ == Direction::Off && stopped(now))
      target = requested_;
    if (target == Direction::Unknown)
      return std::nullopt;
    Command command{++token_, config_.channel, pwm(target), target};
    in_flight_ = command;
    sent_at_ = now;
    return command;
  }
  void complete(uint64_t token, bool accepted, int64_t now)
  {
    if (!in_flight_ || in_flight_->token != token)
      return;
    const auto direction = in_flight_->direction;
    in_flight_.reset();
    if (!accepted)
    {
      fail(direction != Direction::Off);
      return;
    }
    confirmed_ = direction;
    if (direction == Direction::Off)
    {
      neutral_since_ = now;
      stop_since_ = -1;
      stop_samples_ = 0;
      wait_since_ = now;
      state_ = fault_                         ? State::FAILED
               : requested_ == Direction::Off ? State::OFF
                                              : State::WAIT_STOP_BEFORE_REVERSE;
    }
    else if (requested_ == direction && !queued_neutral_)
      state_ = direction == Direction::Forward ? State::FWD : State::REV;
  }
  void output(int pwm_value, int64_t now)
  {
    if (in_flight_ || queued_neutral_ || confirmed_ == Direction::Unknown)
      return;
    if (pwm_value != pwm(confirmed_))
      force_off(now);
  }
  bool satisfied() const
  {
    return !fault_ && !in_flight_ && !queued_neutral_ && confirmed_ == requested_;
  }
  bool failed() const
  {
    return fault_;
  }
  bool current(uint64_t token) const
  {
    return in_flight_ && in_flight_->token == token;
  }
  uint64_t revision() const
  {
    return revision_;
  }
  Direction requested() const
  {
    return requested_;
  }
  Direction confirmed() const
  {
    return confirmed_;
  }
  State state() const
  {
    return state_;
  }
  int channel() const
  {
    return config_.channel;
  }
  bool enabled() const
  {
    return allowed_ && satisfied() && confirmed_ != Direction::Off;
  }
  static const char* label(Direction direction)
  {
    switch (direction)
    {
      case Direction::Off:
        return "off";
      case Direction::Forward:
        return "forward";
      case Direction::Reverse:
        return "reverse";
      default:
        return "unknown";
    }
  }

private:
  int pwm(Direction direction) const
  {
    return direction == Direction::Forward   ? config_.forward
           : direction == Direction::Reverse ? config_.reverse
                                             : config_.neutral;
  }
  void fail(bool send_neutral)
  {
    const bool neutral_pending = in_flight_ && in_flight_->direction == Direction::Off;
    if (in_flight_ && !neutral_pending)
    {
      in_flight_.reset();
      ++token_;
    }
    ++revision_;
    fault_ = true;
    requested_ = Direction::Off;
    state_ = State::FAILED;
    queued_neutral_ = queued_neutral_ || (send_neutral && !neutral_pending);
  }
  Config config_;
  State state_{State::TRANSITION_TO_OFF};
  Direction requested_{Direction::Off}, confirmed_{Direction::Unknown};
  bool connected_{false}, allowed_{false}, fault_{false}, queued_neutral_{true};
  std::optional<Command> in_flight_;
  uint64_t token_{0}, revision_{0};
  int64_t sent_at_{0}, neutral_since_{0}, wait_since_{0}, stop_since_{-1};
  unsigned stop_samples_{0};
  int sample_source_{-1};
  int64_t sample_stamp_{0}, sample_receipt_{-1};
  uint32_t sample_count_{0};
  bool sample_zero_{false};
};

}  // namespace mowgli_mavros_bridge
