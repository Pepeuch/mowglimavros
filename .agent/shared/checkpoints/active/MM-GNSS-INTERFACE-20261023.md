# MM-GNSS-INTERFACE-20261023 — ACTIVE, software proposal for review

Owner: MowgliMAVROS. Standalone review branch `review/gnss-interface-20261023`,
base `4673f43cb6b2b6b9df0685d36f8f8bb43e6e1456`. No commit or push.
MowgliNext `9686b37556f1d2ba7f7a295eb006f870c86e8095` and all three submodules
remain unchanged. Durable evidence: `docs/validation/gnss-interface-20261023/`.

Confirmed from the operator's digest-addressed image: the sidecar index c57fd393
and both native architectures install GnssStatus RIHS01_6765a75d..., copied from
MowgliNext e789ccc2. BuildKit provenance ties them to MAVROS 4673f43, build
39382305156/attempts/1 on 2026-10-20 morning. Producer fields were added in
MowgliNext 1e1af899 that evening; the other supplied image 7489a4a6 has source
59051b41. No robot connection was made. Exact hashes and metadata are in
image-provenance.json. Do not rediscover this as a speculative Docker cache issue.

Proposal aligns eight public types to MowgliNext 9686b375, GNSS hash
RIHS01_ca48e4f1007c03b476e559280a5d9a5b1b03dd4061b2f8956aea93ad22a17fdf,
copies opaque source_id/incarnation, checks source/peer/generated contracts,
installs the lock, and gates Docker publication with adapter gtests. No freshness,
RTK interpretation, safety threshold or GUI code change. A frozen self-lock is
insufficient against future consumer upgrades: paired-release comparison and
immutable digests are required; MowgliNext workflow/gitlink integration is only
proposed, not applied.

Evidence: baseline's source-root check falsely passes; new contract rejects old
copy/installed hash. Three provenance tests fail before the two mapper copies,
then 17 adapter gtests pass. Six packages built; 224 current JUnit cases pass,
including real GNSS GPS1/GPS2/direct graphs. Cross-prefix DDS receives 20 real
canonical plugin statuses with MowgliNext type support, preserving identity and
stale/no-fix. GUI existing Go8 and telemetry78 tests pass. Initial missing SDK and
geoid setup failures and the stale CTest dashboard failures are retained, not
converted into a green full-run claim.

Next: review patch, paired-release gate and source/image manifest. No candidate
OCI image built or published, so no future digest/commit is asserted. ARM64 image
and runtime acceptance remain open. HARDWARE_REQUIRED: exact physical setup,
safety prerequisites, passive procedure and pass/fail criteria in proposal.md.
No firmware flash, motor, robot, deployment, CI job or remote mutation. Existing
Fleet reservation remains separate. TODO MM-602/MM-603 carry current follow-up.
