# MM-FIRMWARE-PROVIDERS-20261005
Disposition: RETAINED — software extraction and contract evidence.
Repository: /workspaces/mowglimavros
Branch: refactor/firmware-providers
Baseline HEAD: 803ae93fa346e1c501dc2af3452b71477b247aee
Worktree clean at startup. No commit, push, deployment or physical test this task.

## Authorized scope
User confirmed bootstrap/profile plus C++ strategies: MANUAL_CONTROL mapping,
emergency mode/policy, arm/disarm/mode capabilities. Public ROS contract,
MowgliNext, readiness, generic diagnostics, canonical GNSS, power and common
bridge logic must stay unchanged. ESC legacy explicitly deferred.

## Implementation and semantic decisions
- Python immutable `FirmwareProvider` / `BootstrapCapabilities` registry lives in
  `python/mowgli_mavros_bridge/firmware_provider.py`. Profiles remain apm/px4.
  Betaflight/INAV/Mowgli have explicit false bootstrap/detection capabilities and
  fail with not implemented. Auto is selection policy, not a runtime provider.
- Launch `resolve_firmware_provider` resolves auto once. `_backend_nodes` creates
  the same MAVROS + hardware_bridge nodes, injecting canonical concrete name only
  through private bridge process env MAVROS_RESOLVED_FIRMWARE. No new ROS parameter.
- C++ `FirmwareProvider` factory selects ArduPilotProvider/PX4Provider. Both reuse
  CurrentRuntimeProvider, which delegates to existing rover conversion. PX4's
  current y=steering/z=throttle mapping is intentionally preserved, not corrected
  or certified as native PX4 behavior.
- Default emergency HOLD/disarm and all emergency_mode/emergency_disarm overrides
  (including empty mode) are retained. Emergency latch/state transitions, send
  ordering, forwarding and asynchronous confirmation handling remain in bridge.
- C++ capabilities explicitly describe mapping, arm, disarm and mode implementation
  as true; blade implementation and physical actuation validation as false. Common
  command clients consult arm/disarm/mode capabilities; current providers allow
  exactly the existing paths. Metadata does not enable manual/blade control.
- Finite/drive/neutral gates and timestamp/publish remain common. Publishers,
  subscriptions, ROS parameters/services, readiness, generic diagnostics, GNSS,
  power, ESC projection and wheel odometry source unchanged.
- Standalone bridge without private resolved env retains existing Rover strategy.
  Launch always passes resolved canonical name; invalid/unsupported names fail.
- Python package installed by ament_cmake_python; no extra runtime library.

## Validation PASS (Lyrical amd64)
- Configure/build/install against exact existing rc4 dependencies at
  /tmp/mm-gnss-rc4-build/install (UG commit 383caba3de94e16167764393d5a4ef046078b015).
- All nine CTest suites pass: launch, Python provider, heartbeat detector, provider
  graph, GNSS graph, readiness, ESC tracker, C++ command provider, Rover conversion.
- Launch: 11 cases, including exact MAVROS parameters/remaps and single resolution
  shared with C++ provider, default auto and explicit overrides, errors/no fallback.
- Python provider: 4 cases for immutable metadata/capabilities/profile names,
  unsupported descriptors, aliases and concrete-vs-auto selection.
- C++ provider: 5 cases; exact whole-message equality to original conversion across
  648 input/scale/provider combinations, known axes/clipping, factory rejection,
  explicit capabilities and all emergency policy overrides/defaults.
- Provider graph: 8 real-bridge cases with mock MAVROS services in isolated local
  ROS domains. Tests default/mode-only/disarm-only/no-request policies, drive and
  neutral gates, invalid input, service clearing and local-forwarded success even
  when fake FCU confirmation rejects. No serial connection or real FCU commands.
- Current GNSS graph matrix passes all gps1/gps2/direct ownership/mapping cases.
  Additional installed-bridge PX4 provider graph mavros/gps1 passes.
- Installed Python package/local_setup/launch imports pass without source-path aid.
- Installed launch Node configurations compared to baseline 803ae93 in 32 cases
  (explicit/auto firmware, GNSS mode/source, neutral flag): identical except the
  intentional private bridge environment. Emergency extension only changed C++.
- Common bridge methods outside four provider call sites compared byte-for-byte
  with baseline and unchanged. External backend contract tests: 10/10 pass.
- Python AST/XML syntax, README progress freshness and git diff --check pass.
  Repository search for previous firmware environment variable: no hits.

## Dependency evidence — do not rediscover
Initial /tmp/mm-gnss-build caches predated explicit GNSS 2D/3D/DGPS constants. Its
older installed graph test passed but was not current acceptance. Running current
source graph against that cache failed for absent FIX_TYPE_2D_FIX; rebuilding the
adapter with the old UG cache failed similarly. No source or pin was changed to
fit it. Reused verified rc4 tree documented in active/MM-GNSS-CANONICAL-ADAPTER-
20261003.md; current bridge and current graph then pass. Do not use stale cache
success as current GNSS contract evidence.

## Documentation / remaining work
Component README documents provider boundaries and capabilities; root README and
TODO synchronized. Prior Rock bootstrap proof is for preceding build, retained
separately; no new physical actuation, native PX4 mapping or release acceptance is
claimed. GNSS transients and wheel calibration remain prior independent work.

Exact next: review/commit this selected patch; defer ESC provider extraction to the
next change and keep separate physical/transport/actuation acceptance gates.
