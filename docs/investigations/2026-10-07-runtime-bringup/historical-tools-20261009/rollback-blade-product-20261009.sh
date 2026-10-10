#!/bin/bash
set -euo pipefail
test "$(id -u)" = 1000
test "$(hostname)" = rock-5b
docker image inspect sha256:281e47a4fb7f18ca1bcd72fa2a52141241624bc81c2bd44611beb7ac0d1df913 >/dev/null
test -s /tmp/mowgli-blade-product-rollback-20261009.Xng0jP/env-before
cp /tmp/mowgli-blade-product-rollback-20261009.Xng0jP/env-before /home/pepeuch/mowglinext/docker/.env
cd /home/pepeuch/mowglinext/docker
MAVROS_IMAGE=sha256:281e47a4fb7f18ca1bcd72fa2a52141241624bc81c2bd44611beb7ac0d1df913 docker compose -p install up -d --no-deps --pull never --force-recreate mavros
docker inspect --format '{{.Image}} {{.State.Running}}' mowgli-mavros
# Mandatory after restoration: acquire fresh disarmed/neutral/three-ESC-zero
# telemetry and verify bridge active. This script does NOT ARM or actuate.
