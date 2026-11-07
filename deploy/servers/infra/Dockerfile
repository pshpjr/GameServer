# 인프라 단일 이미지: MySQL + OTel Collector + Prometheus + Grafana.
# 빌드 컨텍스트는 deploy/ 폴더다(저장소 루트의 .dockerignore가 deploy/를 뺀다).
# MySQL 공식 이미지를 바탕으로, 나머지 셋은 공식 이미지의 정적 바이너리를 복사한다.
# 버전은 deploy/local/.env와 같게 둔다.
ARG OTEL_COLLECTOR_IMAGE=otel/opentelemetry-collector-contrib:0.163.2
ARG PROMETHEUS_IMAGE=prom/prometheus:v3.15.1
ARG GRAFANA_IMAGE=grafana/grafana:13.2.4

FROM ${OTEL_COLLECTOR_IMAGE} AS otel
FROM ${PROMETHEUS_IMAGE} AS prometheus
FROM ${GRAFANA_IMAGE} AS grafana

FROM mysql:8.4
COPY --from=otel /otelcol-contrib /opt/otelcol/otelcol-contrib
COPY --from=prometheus /bin/prometheus /opt/prometheus/prometheus
COPY --from=grafana /usr/share/grafana /usr/share/grafana
COPY --from=grafana /etc/grafana/grafana.ini /etc/grafana/grafana.ini

COPY mysql/init/ /docker-entrypoint-initdb.d/
COPY --chmod=755 servers/infra/entrypoint.sh /usr/local/bin/portfolio26-infra
RUN sed -i 's/\r$//' /usr/local/bin/portfolio26-infra /docker-entrypoint-initdb.d/*.sh

ENV GF_PATHS_HOME=/usr/share/grafana \
    GF_PATHS_CONFIG=/etc/grafana/grafana.ini \
    GF_PATHS_DATA=/data/grafana \
    GF_PATHS_LOGS=/data/grafana/log \
    GF_PATHS_PLUGINS=/data/grafana/plugins \
    GF_PATHS_PROVISIONING=/config/grafana/provisioning

# 영속 데이터는 /data 아래(mysql, prometheus, grafana), 설정은 /config 아래에 마운트한다.
ENTRYPOINT ["/usr/local/bin/portfolio26-infra"]
