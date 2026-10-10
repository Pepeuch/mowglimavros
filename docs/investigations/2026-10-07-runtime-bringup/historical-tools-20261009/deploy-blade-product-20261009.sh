#!/bin/bash
set -euo pipefail
test "$(id -u)" = 1000
test "$(hostname)" = rock-5b
test "$(docker inspect --format '{{.Image}}' mowgli-mavros)" = sha256:281e47a4fb7f18ca1bcd72fa2a52141241624bc81c2bd44611beb7ac0d1df913
test "$(docker image inspect --format '{{.Id}}' mowgli-mavros-sidecar:blade-product-20261009)" = sha256:788d583129ffeec5cebd660cff1ddfe15357bae272b7741bf7be76bdba0759e1
test "$(docker inspect --format '{{.State.ExitCode}}' mowgli-blade-product-preflight-20261009-r3)" = 0
test -s /tmp/mowgli-blade-product-rollback-20261009.Xng0jP/env-before
cmp /home/pepeuch/mowglinext/docker/.env /tmp/mowgli-blade-product-rollback-20261009.Xng0jP/env-before
trap 'bash /tmp/mowgli-blade-product-rollback-20261009.Xng0jP/rollback-blade-product-20261009.sh' ERR
source /home/pepeuch/mowglinext/install/lib/env.sh
upsert_env_key /home/pepeuch/mowglinext/docker/.env MAVROS_IMAGE mowgli-mavros-sidecar:blade-product-20261009
cd /home/pepeuch/mowglinext/docker
docker compose -p install up -d --no-deps --pull never --force-recreate mavros
docker inspect --format '{{.Image}} {{.State.Running}} {{.State.StartedAt}}' mowgli-mavros
