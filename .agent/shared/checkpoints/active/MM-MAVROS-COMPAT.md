# MM-MAVROS-COMPAT

Status: ACTIVE

Repository: Pepeuch/mowglimavros

## Objective

Make mowglimavros software-compatible with:

- Kilted amd64/arm64
- Lyrical amd64/arm64
- MAVROS 2.15.1

while keeping one source implementation and preserving Kilted as the primary target.

## Established baseline

Read first:

.agent/shared/checkpoints/retained/MM-MAVROS-AUDIT-20260907.md

Do not rediscover that audit unless a listed dependency changed.

## Authorized software scope

1. Fix MAVROS launch inclusion.
2. Replace obsolete ament_target_dependencies usage.
3. Use ConstSharedPtr where required by current rclcpp APIs.
4. Raise only CMake minima required for Lyrical compatibility.
5. Segment RTCM publications to <=720 bytes.
6. Parameterize ROS_DISTRO.
7. Pin MAVROS 2.15.1 source/tag/commit.
8. Pin or explicitly verify MAVLink.
9. Install GeographicLib once from reproducible input.
10. Remove distro-specific Boost ABI assumptions.
11. Update build.sh.
12. Add Kilted/Lyrical CI matrix.
13. Preserve amd64 + arm64.

## Explicitly out of scope

- MowgliNext integration
- Pixhawk parameter tuning
- actuator behaviour changes
- final hardware mappings
- .pyc cleanup
- unrelated documentation cleanup
- unrelated warning cleanup

## Validation order

1. syntax/static launch checks
2. focused RTCM tests
3. package tests
4. Kilted workspace build
5. Lyrical workspace build
6. Kilted amd64 image
7. Lyrical amd64 image
8. Kilted arm64 image
9. Lyrical arm64 image
10. smoke test:
   - MAVROS version = expected
   - expected topics/services exist
   - MAVROS launch succeeds

Do not jump directly to expensive multiarch builds when an earlier layer fails.

## Acceptance

Software compatibility is complete only when:

- Kilted passes
- Lyrical passes
- MAVROS 2.15.1 is verifiably used
- RTCM >720 input is handled without data loss/reordering
- backend launch succeeds
- amd64 and arm64 images build
- no Kilted/Lyrical-specific C++ fork is required

Hardware-dependent behaviour remains separate.

## Exact next step

Implement the smallest SOFTWARE_NOW compatibility patch starting with:

1. MAVROS launch filename
2. CMake Lyrical compatibility
3. RTCM segmentation

Then run focused validation before container restructuring.