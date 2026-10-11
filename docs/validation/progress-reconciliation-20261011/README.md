# Reconciliation TODO / project progress — 2026-10-11

Owner: MowgliMAVROS, main at8eecc57d4866052be102acad91bb357fe07bb938.
Existing documentation-ownership edits were preserved. No code, functional policy,
branch, commit, push, gitlink or hardware change. The existing generator and its
counting formula are unchanged; TODO.md remains canonical.

| Item | Previous display | Reconciled display | Evidence / remaining acceptance |
|---|---|---|---|
| MM-201 | TODO despite a second DONE statement in the same section | DONE, software boundary scope | NtripClientNode::split_rtcm_chunks/publish_rtcm and RtcmChunking.PreservesRequiredBoundaryInputs; historical focused Kilted amd64 build/gtest PASS dated2026-09-08. Physical RTCM delivery stays MM-801/HW-MAV-005. |
| MM-604 | TODO despite implemented/checkmarked tasks | DONE software; HARDWARE_PENDING | MM-ESC-ODOMETRY-20261005 records76 wheel/source cases, native graph and bridge/contracts PASS. Ground-distance/track/calibration and target acceptance remain open. |
| MM-608 | IN PROGRESS plus withdrawn GUI SOFTWARE_REQUIRED | IN PROGRESS: software complete, physical HARDWARE_REQUIRED | main8eecc57 final bridge18 suites112 XML tests; software checkboxes now explicit. Exact-baseline HOLD/servo, zero wheels, GUIDED units/signs, races and independent companion-loss stopping still unaccepted. |
| MM-601 / MM-701 | some completed software dependencies unchecked; old already-armed-only command description | software prerequisites checked, historical command baseline labelled, remaining acceptance explicit | No freeze/production/physical acceptance claimed. Historical Next blade policy stays unchanged; idle-ON proposal withdrawn by user. |

Source boundary implementation and gtest were inspected at the current HEAD;
no NTRIP/ROS rebuild or physical RTCM injection occurred during bookkeeping.
Historical Kilted NTRIP validation used mavros-msgs2.15.0; then-production pin
was MAVROS2.15.1/22ae5b7c, current pin2.16.0/5c68b905ab30de6ce630822dc46c33467e8f23ea.
Final FCU baseline/results are retained in ../fcu-main-commit-20261011/README.md;
physical procedures are in ../../architecture/fcu-session-controller.md.

Progress: 13/28 (46.43%) ->14/28 (50.00%) verified. DONE19->21;
TODO4->2; IN PROGRESS2/BLOCKED1/DEFERRED2 unchanged. Hardware-pending7
(previous6): MM-604 is explicitly qualified and does not increase the verified
numerator. The separate MM-608 HARDWARE_REQUIRED remains IN PROGRESS; no physical
gate is counted as complete. All28 stable work-item IDs are conserved.

Executed UID1000:
- python3 tools/update_readme_progress.py: regenerated README and SVG.
- python3 tools/update_readme_progress.py --check: PASS, deterministic output.
- python3 tools/test_update_readme_progress.py: 3/3 PASS.
- python3 tools/test_external_backend_contract.py: 10/10 PASS.
- Accounting assertions: unique28 IDs; MM-201 DONE software, MM-604 hardware
  qualifier, MM-608 IN PROGRESS;14 verified and21 DONE: PASS.
- git diff --check: PASS. Source and tests/generator unchanged.

baseline.json preserves pre-edit input hashes and existing dirty state.
results.json records current validation. Existing documentation-ownership
verification remains a historical snapshot of the preceding task; these deliberate
TODO/README/SVG updates change its input hashes and are recorded here instead
of rewriting that earlier preservation evidence.

Shared handoff: .agent/shared/checkpoints/retained/MM-PROGRESS-RECONCILIATION-20261011.md.
Next: review local documentation changes; no commit/push authorized.
