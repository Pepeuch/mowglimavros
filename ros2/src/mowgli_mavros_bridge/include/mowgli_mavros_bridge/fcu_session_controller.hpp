#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace mowgli_mavros_bridge
{
// MAVROS command sequencer only. MowgliNext owns the functional demand;
// SafetyState/BladeControl/ArduPilot retain their existing safety decisions.
// The serial identifies a local RPC callback, never a user authorization.
class FcuSessionController
{
public:
  enum class Kind
  {
    Mode,
    Arm
  };
  struct Command
  {
    uint64_t serial;
    Kind kind;
    std::string mode;
    bool arm;
  };
  const std::string& requested_mode() const
  {
    return wanted_mode_;
  }
  const std::string& confirmed_mode() const
  {
    return mode_;
  }
  bool requested_arm() const
  {
    return wanted_arm_;
  }
  bool confirmed_arm() const
  {
    return armed_;
  }
  const std::string& error() const
  {
    return error_;
  }
  bool failed() const
  {
    return mode_failed_ || arm_failed_;
  }
  bool current(uint64_t serial) const
  {
    return pending_ && pending_->serial == serial;
  }
  bool command_pending() const
  {
    return pending_.has_value();
  }
  bool command_acknowledged() const
  {
    return pending_ && acked_;
  }
  bool disarm_pending() const
  {
    return pending_ && pending_->kind == Kind::Arm && !pending_->arm;
  }
  bool ready() const
  {
    return connected_ && !pending_ && mode_ == wanted_mode_ && armed_ == wanted_arm_;
  }

  void demand(const std::string& mode, bool arm, bool safety_disarm = false)
  {
    if (wanted_mode_ == mode && wanted_arm_ == arm && safety_disarm_ == safety_disarm)
      return;
    wanted_mode_ = mode;
    wanted_arm_ = arm;
    mode_failed_ = arm_failed_ = false;
    error_.clear();
    if (safety_disarm && !safety_disarm_)
    {
      // Safety DISARM must not wait for an unrelated MODE/ARM transaction.
      // The wire request cannot be retracted; late callbacks are ignored and
      // any subsequent observed ARM is reconciled by a corrective DISARM.
      pending_.reset();
      force_disarm_ = true;
    }
    safety_disarm_ = safety_disarm;
  }
  void observe(bool connected, bool armed, const std::string& mode)
  {
    if (!connected)
      pending_.reset();
    if (safety_disarm_ && armed && !armed_)
    {
      force_disarm_ = true;
      arm_failed_ = false;
    }
    connected_ = connected;
    armed_ = armed;
    mode_ = mode;
    // An actual late FCU confirmation fulfills the current target without a
    // retry or new functional request. A mismatching state retains the failure.
    if (connected_ && mode_ == wanted_mode_)
      mode_failed_ = false;
    if (connected_ && armed_ == wanted_arm_)
      arm_failed_ = false;
    settle();
  }
  void tick(int64_t now)
  {
    // This bounds only an outstanding RPC/confirmation, not session liveness.
    if (pending_ && now - sent_at_ >= 3000)
      fail("confirmation_timeout");
  }
  std::optional<Command> next(int64_t now, bool mode_service, bool arm_service, bool allow_arm)
  {
    tick(now);
    if (!connected_ || pending_)
      return std::nullopt;
    Command command{0, Kind::Mode, wanted_mode_, false};
    if (safety_disarm_ && (force_disarm_ || armed_) && !arm_failed_)
    {
      if (!arm_service)
        return std::nullopt;
      command.kind = Kind::Arm;
      force_disarm_ = false;
    }
    else if (mode_ != wanted_mode_ && !mode_failed_)
    {
      if (!mode_service)
      {
        // HOLD unavailability cannot obstruct the existing DISARM path.
        if (wanted_arm_ || !armed_ || arm_failed_ || !arm_service)
          return std::nullopt;
        command.kind = Kind::Arm;
      }
    }
    else if (armed_ != wanted_arm_ && !arm_failed_)
    {
      // An activation never proceeds past a failed MODE. Ending a session can
      // still DISARM even when HOLD has failed or timed out.
      if ((wanted_arm_ && (mode_ != wanted_mode_ || !allow_arm)) || !arm_service)
        return std::nullopt;
      command.kind = Kind::Arm;
      command.arm = wanted_arm_;
    }
    else
      return std::nullopt;
    command.serial = ++serial_;
    pending_ = command;
    acked_ = false;
    sent_at_ = now;
    return command;
  }
  void complete(uint64_t serial, bool accepted, int64_t now)
  {
    tick(now);
    if (!current(serial))
      return;
    if (!accepted)
    {
      fail("request_refused");
      return;
    }
    acked_ = true;
    settle();
  }

private:
  void settle()
  {
    if (!pending_ || !acked_)
      return;
    const bool confirmed =
        pending_->kind == Kind::Mode ? mode_ == pending_->mode : armed_ == pending_->arm;
    if (confirmed)
    {
      pending_.reset();
      if (!failed())
        error_.clear();
    }
  }
  void fail(const char* reason)
  {
    if (!pending_)
      return;
    const bool mode = pending_->kind == Kind::Mode;
    if (mode && pending_->mode == wanted_mode_)
      mode_failed_ = true;
    else if (!mode && pending_->arm == wanted_arm_)
      arm_failed_ = true;
    error_ = std::string(mode ? "mode:" : "arming:") + reason;
    pending_.reset();
  }
  std::string wanted_mode_{"HOLD"}, mode_, error_;
  bool wanted_arm_{false}, armed_{false}, connected_{false}, safety_disarm_{false};
  bool force_disarm_{false}, mode_failed_{false}, arm_failed_{false}, acked_{false};
  uint64_t serial_{0};
  int64_t sent_at_{0};
  std::optional<Command> pending_;
};
}  // namespace mowgli_mavros_bridge
