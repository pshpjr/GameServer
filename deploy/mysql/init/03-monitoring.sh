#!/bin/bash
#  otel-collector의 mysql receiver가 쓰는 읽기 전용 계정. 값은 deploy/local/monitoring.env에서 온다.
#  초기화 때 entrypoint가 실행한다. 이미 있는 DB에는 docker exec로 이 스크립트를 직접 실행한다.
set -eu
mysql --protocol=socket -uroot -p"${MYSQL_ROOT_PASSWORD}" <<EOSQL
CREATE USER IF NOT EXISTS '${MYSQL_MONITOR_USER}'@'%' IDENTIFIED BY '${MYSQL_MONITOR_PASSWORD}';
GRANT PROCESS, REPLICATION CLIENT ON *.* TO '${MYSQL_MONITOR_USER}'@'%';
GRANT SELECT ON performance_schema.* TO '${MYSQL_MONITOR_USER}'@'%';
EOSQL
