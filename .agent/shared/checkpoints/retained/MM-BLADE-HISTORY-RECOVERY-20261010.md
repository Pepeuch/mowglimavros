# MM-BLADE-HISTORY-RECOVERY-20261010

Disposition: RETAINED — recovered review evidence; no product change.
Baseline: MowgliMAVROS main9f62ba4b; parent feat/mavros-refresh582e6fe9.

Authoritative report and review diff:
`docs/investigations/2026-10-07-runtime-bringup/blade-oct09-history-recovery.md`.

- Exact OFF/FWD/REV patch survives in refs/stash c93548f36d12549653bc35e26d2d33bf558f76eb,
  created9October11:02:35UTC before reset to HEAD. Third parent45d7740 holds
  new headers/tests/checkpoints. Not in published branch history; stash is a
  Git commit object, not an integrated feature commit. No orphan feature commit found.
- 7466cc77 is7October and retains send_arm_command(blade_authorized).
  Its node equals currentmain9f62ba4. Remote feat/mavros-refresh is4 commits behind
  main; those commits do NOT include the independent blade controller.
- Stash source hashes match local/robot Oct9 archives: nodea8cbaee9…, FSM206a8c27….
  Old ARM64 image788d5831… remains present; current activeimagecfb4c921…,
  installedbinary13f26d44…, started10October12:43:52UTC. Source commit of active
  binary is not established by labels/clone; do not infer it from latest.
- Exact review diff has tracked changes plus7 new code/test files,16files total.
  No application. Historical checkpoints recovered into separate review folder;
  Oct10 audit reports/data/scripts untouched.
- Fresh local tests:14puregtests +2staticcontracts PASS, C++17warnings-as-errors,
  git apply --check and diff --check PASS. No full ROS build/graph rerun claimed.
- Historical software/build/graph PASS remains baseline-specific. Physical
  startup/OFF passed; FWD product refused before nonneutral output, other target
  trials not run. Companion-loss failsafe/exclusive ownership remain unresolved.

No reset/cherry-pick/merge/stash apply/pop/fetch/deploy/restart/FCU command,
code edit, commit or push. Next: operator reviews recovered diff before any
selective reintegration; resume requested10Hz/authorization/backend regressions
on existing implementation, not a new invented blade path.
