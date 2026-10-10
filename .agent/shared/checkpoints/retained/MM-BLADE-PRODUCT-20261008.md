# MM-BLADE-PRODUCT-20261008

Disposition: RETAINED — software validation; product deployed2026-10-09,
startup/OFF target PASS, first FWD request refused; STOP before nonneutral output.
MowgliMAVROS main9f62ba4b + uncommitted working patch. No commit/push.

Authoritative checkpoint and exact evidence:
`docs/investigations/2026-10-07-runtime-bringup/blade-product-fwd-rev-implementation.md`.
Read it together with blade-charge-ab-acceptance.md before further blade work.

- Latest target state: blade-product image788d5831… deployed2026-10-09,
  startup/OFF now PASS after corrected diagnostics. Explicit bench ARM accepted,
  first FWD request false; only183/3/1500 observed, no1450/1550, all rawRPM0.
  Cleanup disarmed/MANUAL/1500x3/RPM0x3/intentoff/CANoff confirmed; rescue normal.
  Read blade-product-arm64-deployment.md before any next target action.
  Exact OFF-only281e47a4… rollback prepared, not executed.
- Native bench established SERVO3/ESC2, neutral1500, raw signed feedback and
  CHARGE_NO_EFFECT on the limited A/B test. Mechanical1550 rear/1450 forward
  is installation evidence only, not a universal mapping or nominal speed.
- Product BladeControl owns OFF/FWD/REV only, no ARM/DISARM or mode policy.
  Parameters3/1500/1450/1550 are configurable/read-only at startup, baseline only.
- Neutral-before-inversion requires fresh, distinct ESC zero acquisitions;
  cached republication, PWM and ACK cannot prove stop. Stale/reject/timeouts
  neutralize/cancel ON; deadlines and safety fence late replies; failed requests
  do not retry by tick. Startup/disconnect/reconnect/safety explicitly neutralize.
- Deferred mower responses reflect actual MAVROS ACK, not local queue acceptance.
  Status separates command intention/ACK confirmation from unchanged ESC telemetry.
- Software PASS: build6 packages, bridge16 suites, ESC5, power1, GNSS adapter1;
  14 new state gtests; provider and blade graphs also pass installed binary;
  clang-format18/diff checks. SDK MAVROS2.16.0 commit5c68b905…, no hardware access.
- Source nodeSHA a8cbaee9bae1fe050a8479ec72e5100745a283003c30414e13901e06649584cf;
  installed binarySHA1d1bf5b479c10d3605cc9f78f39fd247b91c6661c33d374085cd58215b49845c.
- Limits remain: indistinguishable identical external command racing a pending
  local wire mirror (no exclusive ownership proof); total-companion-loss FCU
  failsafe unresolved; signed transport through EscObservation not implemented.

ARM64 build and isolated blade/provider graphs PASS; no product code change.
STOP target on first FWD refusal, no nonneutral commands; REV/inversion/safety
not run. Diagnosis must distinguish source-qualified ESC freshness (legacy
distinct samples ~1Hz) and competing OFF/invalidation, not weaken identity guards.
Next after operator direction: diagnose refusal before correction or new trials.
Do not reuse raw-command bench tooling unchanged with the new product owner.
