# MM-MAVROS-COMPAT

Status: ACTIVE

Repository: Pepeuch/mowglimavros

## Objective

Make mowglimavros software-compatible with:

- Kilted amd64/arm64
- Lyrical amd64/arm64
- MAVROS 2.15.1

while keeping one source implementation and preserving Kilted as the primary target.

## Active lot — deterministic build foundations

Authorized on 2026-09-07 against repository HEAD
`faddf0de3aa03f6846df88f5d4022b495d37967a`.

Current priority is limited to:

- MM-301 — parameterize `ROS_DISTRO`, Kilted by default;
- MM-302 — build MAVROS 2.15.1 from commit
  `22ae5b7cc7cdb4cb9c2070a8213c72dae445a23e` and pin/verify MAVLink;
- MM-303 — prepare the GeographicLib release assets and consume them once,
  with SHA256 verification;
- MM-304 — remove Noble-specific Docker dependency assumptions;
- MM-401/MM-402 — propagate pins through `build.sh` and separate CI caches.

The release tag `geographiclib-datasets-v1` must only be prepared locally.
Publishing the GitHub release remains explicitly unauthorized.

### Deterministic input established

MAVROS 2.15.1 requires exactly these GeographicLib archives:

- `egm96-5.tar.bz2` → `geoids/egm96-5.pgm`, SHA256
  `c46224f8f723dc915d97179f4e1580a98d6c742fe2b82cd8fef0ecaaad13e614`;
- `egm96.tar.bz2` → `gravity/egm96.egm`, SHA256
  `6fea4c6bd56ff8ac53dbdad8d5dd505c855471d0354c4abc5c5fe048bf8350c1`;
- `emm2015.tar.bz2` → `magnetic/emm2015.wmm`, SHA256
  `8e71a9704c5f2714bb65581df68e30f0d84d0ad17286d00efb782e7232334c3f`.

The ignored local upload bundle is
`.agent/checkpoints/geographiclib-datasets-v1/`; its tracked compact manifest
is `ros2/geographiclib-datasets/manifest.yaml`.

Current APT MAVLink versions are verified against the `2026.8.8` prefix. This
is an explicit version check, not an APT snapshot pin. APT indexes and ROS base
images remain moving inputs.

`libboost-system1.83.0` remains an explicit runtime dependency: a simulation of
the runtime ROS dependency set did not install Boost.System, while the NTRIP
client is linked against it. It is currently available on both Kilted and
Lyrical and is therefore retained as necessary, not as an assumed transitive
dependency.

OCI base-image pinning remains deliberately deferred. If required, add a
per-distro `ROS_BASE_DIGEST` build argument in CI, using the multi-architecture
index digests observed on 2026-09-07:

- Kilted: `sha256:0030f32dc8a71ef8401c89470db6003c779f036f532d40195790f58f0001902d`;
- Lyrical: `sha256:0eea195bb91662a7dfbd6c6b026c209aef1d091e55ce37733e54509580ddf2c8`.

This preserves amd64 and arm64 under a single index, but requires an explicit
digest-update process; it is not applied automatically in this lot.

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
- MM-101 launch correction in this lot
- MM-102/MM-103 CMake and callback corrections in this lot
- MM-201 RTCM segmentation in this lot

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

Await authorization to publish the prepared `geographiclib-datasets-v1` release.
Once available, validate GeographicLib, MAVROS source, workspace and image in
the declared Kilted-then-Lyrical, amd64-then-arm64 order.
