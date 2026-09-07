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

Resumed on 2026-09-07 at repository HEAD
`6faa4fe828f2904857994d906fda8adc9748cd33`.

Current priority is limited to:

- MM-301 — parameterize `ROS_DISTRO`, Kilted by default;
- MM-302 — build MAVROS 2.15.1 from commit
  `22ae5b7cc7cdb4cb9c2070a8213c72dae445a23e` and pin/verify MAVLink;
- MM-303 — prepare the GeographicLib release assets and consume them once,
  with SHA256 verification;
- MM-304 — remove Noble-specific Docker dependency assumptions;
- MM-401/MM-402 — propagate pins through `build.sh` and separate CI caches.

The release tag `geographiclib-datasets-v1` is prepared locally. Its
publication has explicit, asset-limited authorization, but must not begin
until `gh` has been explicitly authenticated from the development container.

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

## Development container — validated

The uncommitted `.devcontainer` is intentionally separate from the runtime
image. It defaults to `ROS_DISTRO=kilted` and retains that build argument for
the future Lyrical target without a second Dockerfile.

The official ROS base already provides `ubuntu` UID/GID 1000, which is used as
the remote user. The container mounts the host Docker socket and starts no
Docker daemon; `docker.io` and `docker-buildx` are clients only. The named
`mowglimavros-gh-config` volume persists `/home/ubuntu/.config/gh`; both the
image and Dev Containers set `GH_CONFIG_DIR` to that location. No credential
is in the image or repository. `gh` and `openssh-client` are installed.

The Kilted devcontainer build passed. In the validation container, the host
Docker daemon/socket, `docker buildx`, Git, `gh --version`, SSH, and sourced
ROS Kilted passed. Native colcon has no `--version` subcommand; use
`colcon --log-base /tmp/colcon-log version-check` where a bind-mounted
workspace is not writable.

## Exact next step

The CI configuration for isolated ccache persistence is now statically
validated, but the required two GitHub-hosted runs cannot occur until this
uncommitted lot is authorized for commit and push. With that authorization,
run the empty-cache CI build followed by the Mowgli-source-only CI build; do
not start arm64, hardware, or MowgliNext work beforehand.

## Release and Kilted amd64 evidence — 2026-09-07

Release `geographiclib-datasets-v1` is published at
https://github.com/Pepeuch/mowglimavros/releases/tag/geographiclib-datasets-v1
with exactly these four assets: `egm96-5.tar.bz2`, `egm96.tar.bz2`,
`emm2015.tar.bz2`, and `SHA256SUMS`.

Every asset was redownloaded from its GitHub Release URL. `SHA256SUMS` was
byte-identical to the staged file and `sha256sum --check SHA256SUMS` passed.
The validated archive SHA256 values are:

- `egm96-5.tar.bz2`: `c46224f8f723dc915d97179f4e1580a98d6c742fe2b82cd8fef0ecaaad13e614`
- `egm96.tar.bz2`: `6fea4c6bd56ff8ac53dbdad8d5dd505c855471d0354c4abc5c5fe048bf8350c1`
- `emm2015.tar.bz2`: `8e71a9704c5f2714bb65581df68e30f0d84d0ad17286d00efb782e7232334c3f`

The Dockerfile `geographiclib-data` target downloaded the real release assets,
validated their expected SHA256 values, and extracted all required dataset
files. The Kilted amd64 runtime image then passed, producing local image
`mowgli-mavros-sidecar:kilted-amd64-local`
(`sha256:53f3329c1be276adf94dfb6eab01d180b185b9d801919afb37436565ff99012b`).
Its MAVROS source tag, commit, and package version checks all passed for
2.15.1 / `22ae5b7cc7cdb4cb9c2070a8213c72dae445a23e`. The workspace emitted the
already-known `ament_target_dependencies()` deprecation warning only.

## Lyrical amd64 and cache audit — 2026-09-07

Lyrical amd64 first exposed the removed
`ament_target_dependencies()` macro in this repository's bridge CMake. The
minimal shared-source correction replaces that call with the imported targets
recommended by Kilted's deprecation diagnostic; there is no ROS-distro branch
or compatibility shim. The Lyrical runtime image now passes as
`mowgli-mavros-sidecar:lyrical-amd64-local`
(`sha256:43943f4b7a32f348c36b3fbf76ae129d3701123edd5f912fbcd9556d4cd33b30`,
1,391,123,656 bytes). Kilted needs a later revalidation because this shared
CMake source changed; it was not rerun under the no-retest constraint.

Cache evidence:

- `geographiclib-data` has no Mowgli source `COPY` input. Its only variable
  input is `GEOGRAPHICLIB_DATASET_VERSION` plus its installation instructions;
  a Lyrical source-only change left both GeographicLib stages `CACHED`.
- MAVROS checkout/pin and build stages are upstream/source layers before
  `COPY ros2/src/`; they remained `CACHED` when only Mowgli source changed.
- ccache is now isolated as
  `ccache-${ROS_DISTRO}-${TARGETARCH}-${MAVROS_VERSION}` and capped at 512 MiB
  (reduced from the initial 2 GiB after the 89 MiB measurement below).
  For Lyrical amd64/2.15.1, the cold MAVROS build measured 1,388 cacheable
  calls, 4 hits, 1,384 misses, 38 uncacheable calls, and 87 MiB. The following
  workspace build ended at 1,533 cacheable calls, 82 hits, 1,451 misses,
  95 uncacheable calls, and 89 MiB. BuildKit reports the isolated ccache mount
  at 85.02 MB; the total local BuildKit cache was 12.45 GB.
- A second strictly identical Lyrical amd64 build marked both GeographicLib
  stages, MAVROS checkout/pin/build, and the workspace build `CACHED`; no
  dataset request or MAVROS recompilation occurred.
- GitHub Actions layer caches are correctly scoped by ROS distro, MAVROS
  version and architecture. `type=gha,mode=max` persists those layers, but it
  does not by itself persist `RUN --mount=type=cache` data. CI ccache
  persistence remains an explicit open item requiring a cache-mount mechanism
  (for example BuildKit Cache Dance) rather than sharing incompatible objects.

## Kilted amd64 modern-CMake revalidation — 2026-09-07

The same imported-target `target_link_libraries()` CMakeLists used by Lyrical
passed the Kilted amd64 runtime build, producing
`mowgli-mavros-sidecar:kilted-amd64-local`
(`sha256:ba694bba69e90a7dc4e7ebc804205bf15e537a79dc33ac779b3ea7419f152624`).
There is no ROS-distro conditional and no ament compatibility shim. The
GeographicLib and MAVROS checkout/pin stages were `CACHED`; the workspace
compiled successfully with no new functional/build regression evident.

## CI ccache persistence implementation — 2026-09-07

The workflow now retains BuildKit layers through its existing, separately
scoped `type=gha,mode=max` cache and persists cache mounts explicitly with
`actions/cache@v4` plus `reproducible-containers/buildkit-cache-dance@v3`.
The mount id is exactly
`ccache-${ROS_DISTRO}-${TARGETARCH}-${MAVROS_VERSION}`. The GitHub cache key
also includes runner OS, ROS distro, architecture, MAVROS version, pinned
MAVROS commit, and Dockerfile hash:
`ccache-${runner.os}-${ROS_DISTRO}-${architecture}-${MAVROS_VERSION}-${MAVROS_COMMIT}-${Dockerfile hash}`.
This prevents cross-distro and cross-architecture object reuse; a Dockerfile
or MAVROS pin change deliberately starts a fresh compiler cache.

Measured amd64/2.15.1 ccache use was 89 MiB (85.02 MB as reported for the
BuildKit mount). `CCACHE_MAXSIZE` is therefore now 512 MiB, leaving more than
five times that measured working set. No evidence supports 1 GiB or 2 GiB for
this workload; 512 MiB avoids a theoretical 8 GiB across four matrix keys.
No local cache was purged. Workflow YAML parsing and `git diff --check` pass.

This is a static implementation checkpoint, not a claim that the remote cache
has been restored. A first empty-cache GitHub Actions run and a second run
with only a Mowgli source change remain blocked on authorization to commit and
push the workflow. They must demonstrate the key above, restored ccache,
ccache hits/misses/size, GeographicLib `CACHED` with no download, and MAVROS
layer `CACHED` before arm64 may begin.

## First GitHub Actions run — pre-BuildKit failure — 2026-09-07

The first real CI run failed before BuildKit started, for one cause only:
`IMAGE_NAME` interpolated `github.repository_owner` as `Pepeuch`, while GHCR
repository names must be lowercase. The workflow now uses the fixed literal
`ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar`. Node 20 and `punycode`
warnings are non-blocking and intentionally out of scope. No multiarch build,
GeographicLib action, MAVROS action, ccache restore/injection, or Cache Dance
validation was exercised by this run. The next real CI run is still required
after commit/push to validate the configured cache persistence.
