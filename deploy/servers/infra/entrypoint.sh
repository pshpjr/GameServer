#!/bin/bash
# 네 서비스를 띄우고 하나라도 끝나면 나머지를 정상 종료한 뒤 컨테이너를 끝낸다.
# 재시작은 Docker 재시작 정책이 컨테이너 단위로 맡는다. 종료 신호(SIGTERM)는 모든 서비스에 전달한다.
set -u

mkdir -p /data/mysql /data/prometheus /data/grafana
chown -R mysql:mysql /data/mysql
install -m 644 /config/mysql.cnf /etc/mysql/conf.d/portfolio26.cnf

pids=()
stop_all() {
    for pid in "${pids[@]}"; do
        kill -TERM "$pid" 2>/dev/null
    done
    wait
}
trap 'stop_all; exit 0' TERM INT

# MySQL 공식 엔트리포인트가 빈 데이터 폴더면 초기화(init SQL 실행)까지 한다.
/usr/local/bin/docker-entrypoint.sh mysqld &
pids+=($!)

/opt/prometheus/prometheus \
    --config.file=/config/prometheus.yml \
    --storage.tsdb.path=/data/prometheus \
    --web.listen-address=:9090 \
    --web.enable-otlp-receiver &
pids+=($!)

/opt/otelcol/otelcol-contrib --config=/config/otel-collector.yaml &
pids+=($!)

/usr/share/grafana/bin/grafana server \
    --homepath=/usr/share/grafana \
    --config=/etc/grafana/grafana.ini \
    cfg:server.http_port=3001 &
pids+=($!)

wait -n
echo "[infra] 서비스 하나가 종료되어 나머지를 종료한다" >&2
stop_all
exit 1
