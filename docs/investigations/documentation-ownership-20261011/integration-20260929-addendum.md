# Historical integration evidence: unique continuation

Disposition RETAINED. No new runtime test or deployment.
Imported from Next docs/MAVROS_INTEGRATION_CHECKPOINT_20260929.md at
baf343ff37719768eaaf61f2bdfaf5e1db6b2078; SHA256 369c1cc0ae84325651b128a47cc021787e5c70496a0b678fbeca20e53ffe1ee6.

September 29 baselines, POWER/VESC/GPS/IMU readings, neutral MANUAL_CONTROL capture,
stream rates, bridge tests and pending physical gates are already preserved in
[MM-MOWGLINEXT-INTEGRATION-20260929](../../../.agent/shared/checkpoints/active/MM-MOWGLINEXT-INTEGRATION-20260929.md).
The following unique exact sample range and image digests supplement that record.
No duplicated capture/log archive is imported. GUI/BT/orchestration details belong
to the downstream Next integration report.

<!-- source: MowgliNext:docs/MAVROS_INTEGRATION_CHECKPOINT_20260929.md; lines 62-62; sha256 b544e28942a7366ea1067373c329d4267696172714efc22e06dba2f1084aac22 -->

Live POWER capture on 2026-09-30: operator confirms robot was already **on dock**. MAVROS ID0/physical POWER1 ~28.56 V and +1.49–1.51 A, ID1/physical POWER2 ~28.02 V and +0.49–0.50 A; both `charge_state=1`. Per MAVLink `MAV_BATTERY_CHARGE_STATE`, 1 is `OK`, 6 is `UNHEALTHY`, 7 is `CHARGING`. Thus state 1 does not prove charging or non-charging. Operator defines fresh POWER1 voltage >0 V as dock present and the canonical charging indicator for this robot; this is a configured hardware convention, while actual charge current remains separately measurable. The 2026-09-29 off-dock samples must not be merged with today\047s docked state. Future physical POWER1 test: off dock, dock connected without charging, real charge, POWER1 disconnected; compare raw current, voltage, charge state. No FCU parameters changed.

<!-- source: MowgliNext:docs/MAVROS_INTEGRATION_CHECKPOINT_20260929.md; lines 100-100; sha256 d35c4ac8a4e6af3776c321e6160ab6134a1efd02b28d29b2e5ba35bedcfd0076 -->

MowgliMAVROS Git tag `v0.5b` resolves to commit `649e797da1b948b5a4941833cb59bfa8287794e8`, also public `main` HEAD. The Docker workflow emits `lyrical` for successful pushes to `main`, but its metadata rules do not emit a `v0.5b` image tag for Git tag events. GHCR returned not-found for `:v0.5b`. The retained image is therefore the verified `lyrical` index `ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar:lyrical@sha256:96e23dca25c1a854af191e1a3c94ecad977c41af345733d56214cbac08195c94`, with linux/amd64 manifest `sha256:080f2e8ce42b4692a89e72d181620bc08ee7b104215994972f9869ee5798a176` and linux/arm64 manifest `sha256:8183c9102166b96608241bde4a178a834f0efc1c1e03707b9580b517c5871819`. The `main` workflow run for commit `649e797…` completed successfully. Both index/amd64 and arm64 manifests were pulled in the devcontainer only; no image was run or deployed.
