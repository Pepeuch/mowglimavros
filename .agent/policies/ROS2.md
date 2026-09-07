# MowgliMAVROS — ROS2 Policy v1.0

Load when modifying ROS2 nodes, launch files, topics, services, actions,
parameters, QoS, remaps, diagnostics, or lifecycle behaviour.

Root `AGENTS.md` remains authoritative.

## Contract discipline

For every changed ROS surface, identify producer, consumer, type, name/remap,
QoS, lifecycle, freshness/timeout semantics, and failure behaviour.

When the change participates in backend replacement, also load `BACKEND.md`.

Do not silently rename or reinterpret externally consumed topics/messages.

Do not publish cached/old state as if it were a fresh hardware observation unless
the contract explicitly defines that behaviour.

Avoid duplicate publishers when only one backend should own a hardware-facing
surface.

## Launch/config

Backend selection must be deterministic from configuration/environment.

Check that launch conditions, remaps, parameters, namespaces, and container/node
composition do not accidentally activate both backends or leave partial overlay
behaviour.

## Validation

Prefer:

1. launch/static contract inspection;
2. focused node tests;
3. topic/type/remap/QoS checks;
4. integration tests;
5. hardware validation only when the remaining claim crosses the physical boundary.

A clean build alone is not proof of ROS graph compatibility.