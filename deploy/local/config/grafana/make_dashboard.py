#  GameServer 메트릭 Grafana 대시보드 JSON 생성. 대시보드는 이 스크립트를 고친 뒤 다시 생성한다.
#  사용: python make_dashboard.py [출력 경로]. 경로를 생략하면 옆의 dashboards/gameserver.json에 쓴다.
import json, sys
from pathlib import Path

SEL = 'job=~"$job", instance=~"$instance"'
GSEL = SEL + ', group=~"$group"'
DS = {"type": "prometheus", "uid": "${datasource}"}
panels = []
state = {"id": 0, "y": 0}


def nid():
    state["id"] += 1
    return state["id"]


def row(title):
    panels.append({"type": "row", "title": title, "id": nid(), "collapsed": False,
                   "gridPos": {"x": 0, "y": state["y"], "w": 24, "h": 1}, "panels": []})
    state["y"] += 1


def line(items):
    x, height = 0, 0
    for w, p in items:
        h = p.pop("_h")
        p["gridPos"] = {"x": x, "y": state["y"], "w": w, "h": h}
        x += w
        height = max(height, h)
        panels.append(p)
    state["y"] += height


def targets(exprs):
    return [{"datasource": DS, "expr": e, "legendFormat": l, "refId": chr(65 + i)} for i, (e, l) in enumerate(exprs)]


def ts(title, exprs, unit="short", desc="", stack=False, h=8, minzero=True):
    custom = {"lineWidth": 1, "fillOpacity": 10, "showPoints": "never", "spanNulls": True}
    if stack:
        custom.update({"stacking": {"mode": "normal", "group": "A"}, "fillOpacity": 40})
    defaults = {"unit": unit, "custom": custom}
    if minzero:
        defaults["min"] = 0
    return {"type": "timeseries", "title": title, "description": desc, "id": nid(), "datasource": DS,
            "targets": targets(exprs), "_h": h,
            "fieldConfig": {"defaults": defaults, "overrides": []},
            "options": {"legend": {"displayMode": "list", "placement": "bottom"},
                        "tooltip": {"mode": "multi", "sort": "desc"}}}


def stat(title, expr, unit="short", decimals=0, desc=""):
    return {"type": "stat", "title": title, "description": desc, "id": nid(), "datasource": DS,
            "targets": targets([(expr, "")]), "_h": 4,
            "fieldConfig": {"defaults": {"unit": unit, "decimals": decimals,
                                         "thresholds": {"mode": "absolute", "steps": [{"color": "green", "value": None}]}},
                            "overrides": []},
            "options": {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": False},
                        "colorMode": "value", "graphMode": "area", "textMode": "auto"}}


def rate(m, sel=SEL, by=""):
    b = f" by ({by})" if by else ""
    return f'sum{b}(rate({m}{{{sel}}}[$__rate_interval]))'


def quantile(q, m, sel, by=""):
    b = f"le, {by}" if by else "le"
    return f'histogram_quantile({q}, sum by ({b})(rate({m}_bucket{{{sel}}}[$__rate_interval])))'


def avg(m, sel, by=""):
    return f'{rate(m + "_sum", sel, by)} / {rate(m + "_count", sel, by)}'


QDESC = "버킷 경계 사이를 선형 보간한 추정치"

row("Overview")
line([
    (4, stat("Sessions", f'sum(net_sessions{{{SEL}}})')),
    (4, stat("Accept /s", rate("net_accepts_total"), decimals=1)),
    (4, stat("Disconnect /s", rate("net_disconnects_total"), decimals=1)),
    (4, stat("Recv packets /s", rate("net_packets_recv_total"))),
    (4, stat("Process CPU", f'max(process_cpu_usage_percent{{{SEL}, mode="total"}})', "percent", 1)),
    (4, stat("Physical memory", f'sum(process_memory_physical_bytes{{{SEL}}})', "bytes", 1)),
])

row("Network")
line([
    (8, ts("Sessions", [(f'sum by (instance)(net_sessions{{{SEL}}})', "{{instance}}")])),
    (8, ts("Accept / Disconnect", [(rate("net_accepts_total"), "accept"),
                                    (rate("net_disconnects_total"), "disconnect")], "ops")),
    (8, ts("Packets", [(rate("net_packets_recv_total"), "recv"), (rate("net_packets_sent_total"), "sent")], "pps",
           "스레드마다 100개 단위로 반영된다")),
])
line([
    (12, ts("Timeouts", [(rate("net_timeouts_total"), "session timeout"),
                         (rate("net_semaphore_timeouts_total"), "semaphore timeout")], "ops", h=7)),
    (12, ts("Send buffer pool", [
        (f'sum(net_sendbuffer_pool_size{{{SEL}}})', "pool size"),
        (f'sum(net_sendbuffer_acquires{{{SEL}}}) - sum(net_sendbuffer_releases{{{SEL}}})', "in use"),
        (f'sum(net_sendbuffer_pool_empty{{{SEL}}})', "pool empty (total)"),
    ], h=7)),
])

row("Process")
line([
    (8, ts("Process CPU", [(f'process_cpu_usage_percent{{{SEL}}}', "{{instance}} {{mode}}")], "percent", h=7)),
    (8, ts("System CPU", [(f'system_cpu_usage_percent{{{SEL}}}', "{{instance}} {{mode}}")], "percent", h=7)),
    (8, ts("Memory", [
        (f'process_memory_physical_bytes{{{SEL}}}', "{{instance}} physical"),
        (f'process_memory_physical_peak_bytes{{{SEL}}}', "{{instance}} physical peak"),
        (f'process_memory_virtual_bytes{{{SEL}}}', "{{instance}} virtual"),
    ], "bytes", h=7)),
])

row("Groups")
line([
    (12, ts("Process time p99", [(quantile(0.99, "group_process_duration_seconds", GSEL, "group"), "{{group}}")],
            "s", QDESC)),
    (12, ts("Process time p50 / avg", [
        (quantile(0.5, "group_process_duration_seconds", GSEL, "group"), "{{group}} p50"),
        (avg("group_process_duration_seconds", GSEL, "group"), "{{group}} avg"),
    ], "s", QDESC)),
])
line([
    (6, ts("Busy time per second", [(rate("group_process_duration_seconds_sum", GSEL, "group"), "{{group}}")],
           "percentunit", "Process에 쓴 시간 / 경과 시간")),
    (6, ts("Jobs /s", [(rate("group_jobs_total", GSEL, "group"), "{{group}}")], "ops")),
    (6, ts("Job queue size", [(f'sum by (group)(group_job_queue_size{{{GSEL}}})', "{{group}}")])),
    (6, ts("Sessions", [(f'sum by (group)(group_sessions{{{GSEL}}})', "{{group}}")], stack=True)),
])
line([
    (24, ts("Enter / Leave", [(rate("group_enters_total", GSEL, "group"), "{{group}} enter"),
                              (rate("group_leaves_total", GSEL, "group"), "{{group}} leave")], "ops", h=7)),
])

row("DB")
line([
    (6, ts("Queue size", [(f'sum(db_queue_size{{{SEL}}})', "queue")])),
    (6, ts("Queries /s", [(rate("db_queries_total", SEL, "result"), "{{result}}")], "ops", stack=True)),
    (6, ts("Error ratio", [(f'{rate("db_queries_total", SEL + ", result=" + chr(34) + "error" + chr(34))} / '
                            f'{rate("db_queries_total")}', "error")], "percentunit")),
    (6, ts("Query latency", [(quantile(q, "db_query_duration_seconds", SEL), f"p{int(q * 100)}")
                             for q in (0.5, 0.95, 0.99)] + [(avg("db_query_duration_seconds", SEL), "avg")],
           "s", QDESC)),
])
line([
    (12, ts("Queue wait", [(quantile(q, "db_queue_wait_duration_seconds", SEL), f"p{int(q * 100)}")
                           for q in (0.5, 0.95, 0.99)] + [(avg("db_queue_wait_duration_seconds", SEL), "avg")],
            "s", "Execute 호출부터 DB 스레드가 쿼리를 시작할 때까지. " + QDESC)),
    (12, ts("Queue wait vs query (avg)", [(avg("db_queue_wait_duration_seconds", SEL), "queue wait"),
                                          (avg("db_query_duration_seconds", SEL), "query")],
            "s", "요청 하나가 큐 대기와 쿼리 실행에 쓴 평균 시간", stack=True)),
])

row("Game")
line([
    (24, ts("Packets by type", [(rate("game_packets_total", SEL, "type"), "{{type}}")], "pps", stack=True)),
])

MSEL = 'job="mysql"'
row("MySQL")
line([
    (6, stat("Threads connected", f'sum(mysql_threads{{{MSEL}, kind="connected"}})')),
    (6, stat("Queries /s", rate("mysql_query_count_total", MSEL), decimals=1)),
    (6, stat("Slow queries /s", rate("mysql_query_slow_count_total", MSEL), decimals=2)),
    (6, stat("Max used connections", f'max(mysql_max_used_connections{{{MSEL}}})')),
])
line([
    (12, ts("Commands /s", [(f'sum by (command)(rate(mysql_commands_total{{{MSEL}, command=~"select|insert|update|delete|update_multi|delete_multi"}}[$__rate_interval]))',
                             "{{command}}")], "ops", stack=True)),
    (12, ts("Row operations /s", [(rate("mysql_row_operations_total", MSEL, "operation"), "{{operation}}")], "ops")),
])
line([
    (6, ts("Threads", [(f'sum by (kind)(mysql_threads{{{MSEL}}})', "{{kind}}")], h=7)),
    (6, ts("Buffer pool", [
        (f'sum(mysql_buffer_pool_pages{{{MSEL}, kind="data"}}) / sum(mysql_buffer_pool_pages{{{MSEL}, kind="total"}})', "data pages / total"),
        (f'1 - {rate("mysql_buffer_pool_operations_total", MSEL + ", operation=" + chr(34) + "reads" + chr(34))} / '
         f'{rate("mysql_buffer_pool_operations_total", MSEL + ", operation=" + chr(34) + "read_requests" + chr(34))}', "hit ratio"),
    ], "percentunit", "hit ratio = 1 - 디스크 읽기 / 논리 읽기 요청", h=7)),
    (6, ts("Row lock waits", [(rate("mysql_row_locks_total", MSEL + ", kind=" + chr(34) + "waits" + chr(34)), "waits /s"),
                              (rate("mysql_row_locks_total", MSEL + ", kind=" + chr(34) + "time" + chr(34)), "wait ms /s")],
           desc="InnoDB row lock 대기 횟수와 대기 시간(ms)", h=7)),
    (6, ts("Connection errors /s", [(f'sum by (error)(rate(mysql_connection_errors_total{{{MSEL}}}[$__rate_interval])) > 0',
                                     "{{error}}")], "ops", h=7)),
])
line([
    (12, ts("Network", [(rate("mysql_client_network_io_bytes_total", MSEL, "kind"), "{{kind}}")], "Bps", h=7)),
    (12, ts("InnoDB I/O /s", [(rate("mysql_operations_total", MSEL, "operation"), "{{operation}}")], "ops", h=7)),
])

DSEL = 'job="dummy"'
row("Dummy")
line([
    (12, ts("Action delay", [(quantile(q, "dummy_action_delay_seconds", DSEL), f"p{int(q * 100)}")
                             for q in (0.5, 0.95, 0.99)] + [(avg("dummy_action_delay_seconds", DSEL), "avg")],
            "s", "요청부터 응답까지. " + QDESC)),
    (12, ts("Dummy sessions", [(f'sum by (instance)(net_sessions{{{DSEL}}})', "{{instance}}")])),
])
line([
    (12, ts("Action delay p99 by type", [(quantile(0.99, "dummy_action_delay_seconds", DSEL, "type"), "{{type}}")],
            "s", QDESC)),
    (12, ts("Action delay avg by type", [(avg("dummy_action_delay_seconds", DSEL, "type"), "{{type}}")], "s")),
])
#  더미를 여러 개 띄워도 instance(컨테이너 ID)별로 한 화면에서 비교한다.
line([
    (12, ts("Dummy CPU", [(f'process_cpu_usage_percent{{{DSEL}, mode="total"}}', "{{instance}}")], "percent",
            "전체 논리 코어 합을 100%로 정규화한 값")),
    (12, ts("Dummy memory", [(f'process_memory_physical_bytes{{{DSEL}}}', "{{instance}}")], "bytes")),
])
#  보낸 요청 수와 응답 대기 수. 대기가 늘면서 보낸 수가 줄면 응답을 기다리느라 요청이 막힌 것이다.
line([
    (12, ts("Requests sent /s by type", [(rate("dummy_requests_total", DSEL, "type"), "{{type}}")], "ops")),
    (12, ts("Requests waiting for response", [(f'sum by (type)(dummy_requests_pending{{{DSEL}}})', "{{type}}")],
            desc="보냈지만 아직 응답을 받지 못한 요청 수")),
])
#  더미가 지금 무엇을 하는지. acting이 아닌 더미가 늘면 멈춘 것이다.
line([
    (12, ts("Dummies by state", [(f'sum by (state)(dummy_players{{{DSEL}}})', "{{state}}")], stack=True,
            desc="접속한 더미를 상태별로 센 수")),
    (12, ts("Dummies not acting", [(f'sum by (state, reason)(dummy_players{{{DSEL}, reason!="acting"}})',
                                   "{{state}} / {{reason}}")],
            desc="행동하지 않는 이유별 수. wait_response는 로그인·레벨 이동 응답을 기다리는 정상 구간이다.")),
])
line([
    (12, ts("Dummies stuck", [(f'sum by (state, reason, over)(dummy_players_sleeping{{{DSEL}}})',
                              "{{state}} / {{reason}} > {{over}}")],
            desc="30초·5분 넘게 행동하지 않은 더미. wait_create·dead가 계속 쌓이면 멈춘 것이다. 30초를 넘기면 DummyStuck 로그가 남는다.")),
    (12, ts("Disconnects /s by reason", [(rate("dummy_disconnects_total", DSEL, "reason"), "{{reason}}")], "ops",
            "더미가 스스로 끊은 이유. destroy_alive는 서버와 상태가 어긋난 것이다.")),
])
line([
    (12, ts("Session leaves /s by kind", [(rate("dummy_leaves_total", DSEL, "kind"), "{{kind}}")], "ops",
            "reset·invalid는 예상 밖 종료")),
    (12, ts("Average session length", [(f'sum(rate(dummy_session_seconds_total{{{DSEL}}}[$__rate_interval])) / '
                                        f'sum(rate(dummy_leaves_total{{{DSEL}}}[$__rate_interval]))', "avg")], "s",
            "끝난 세션의 평균 연결 시간. 설정한 세션 길이보다 훨씬 짧으면 사망 등으로 먼저 끊기는 것이다.")),
])
line([
    (12, ts("Level changes /s", [(f'sum by (from, to)(rate(dummy_level_changes_total{{{DSEL}}}[$__rate_interval]))',
                                 "{{from}} → {{to}}")], "ops")),
    (12, ts("Packets received /s by type", [(rate("dummy_packets_recv_total", DSEL, "type"), "{{type}}")], "ops",
            "서버에서 받은 패킷 종류별 수")),
])

CSEL = 'job="docker", container_name=~"portfolio26-local-.*"'
row("Containers")
line([
    (12, ts("Disk write", [(f'sum by (container_name)(rate(container_blockio_io_service_bytes_recursive_total{{{CSEL}, operation="write"}}[$__rate_interval]))',
                            "{{container_name}}")], "Bps", "docker_stats. cgroup v2라 횟수(IOPS)는 없고 바이트만 있다.")),
    (12, ts("Disk read", [(f'sum by (container_name)(rate(container_blockio_io_service_bytes_recursive_total{{{CSEL}, operation="read"}}[$__rate_interval]))',
                           "{{container_name}}")], "Bps")),
])
line([
    #  container_cpu_utilization_ratio는 Docker Desktop에서 docker stats의 2~6배로 나와 쓰지 않는다.
    (12, ts("Container CPU (cores)", [(f'sum by (container_name)(rate(container_cpu_usage_nanoseconds_total{{{CSEL}}}[$__rate_interval])) / 1e9',
                                       "{{container_name}}")],
            desc="CPU 누적 시간의 증가율. docker stats와 같은 기준으로 1이 코어 하나")),
    (12, ts("Container memory", [(f'container_memory_usage_total_bytes{{{CSEL}}}', "{{container_name}}")], "bytes")),
])
#  컨테이너 네트워크 바이트. TCP/IP 헤더·ACK를 포함한 실제 전송량이다.
DUMMY_NET = 'container_name=~"portfolio26-local-dummy[0-9]*-1"'
line([
    (12, ts("Network send", [(f'sum by (container_name)(rate(container_network_io_usage_tx_bytes_total{{{CSEL}}}[$__rate_interval]))',
                              "{{container_name}}")], "Bps", "컨테이너가 보낸 바이트. 게임 서버는 클라이언트 하향 + DB 요청")),
    (12, ts("Network receive", [(f'sum by (container_name)(rate(container_network_io_usage_rx_bytes_total{{{CSEL}}}[$__rate_interval]))',
                                 "{{container_name}}")], "Bps", "컨테이너가 받은 바이트. 게임 서버는 클라이언트 상향 + DB 응답")),
])
line([
    (24, ts("Bandwidth per dummy session", [
        (f'sum(rate(container_network_io_usage_tx_bytes_total{{job="docker", {DUMMY_NET}}}[$__rate_interval])) * 8 / sum(net_sessions{{job="dummy"}})', "upstream (client → server)"),
        (f'sum(rate(container_network_io_usage_rx_bytes_total{{job="docker", {DUMMY_NET}}}[$__rate_interval])) * 8 / sum(net_sessions{{job="dummy"}})', "downstream (server → client)"),
    ], "bps", "더미 컨테이너 송수신 ÷ 더미 세션 수. 헤더·ACK 포함. OTLP 내보내기도 섞이지만 작다", h=7)),
])


def var(name, query, current):
    return {"name": name, "type": "query", "datasource": DS, "query": {"query": query, "refId": name},
            "definition": query, "refresh": 2, "multi": True, "includeAll": True, "allValue": ".*",
            "current": current, "sort": 1}


ALL = {"text": "All", "value": "$__all"}
dashboard = {
    "uid": "gameserver-netlib", "title": "GameServer", "tags": ["gameserver", "otel"],
    "editable": True, "schemaVersion": 39, "refresh": "10s", "time": {"from": "now-30m", "to": "now"},
    "templating": {"list": [
        {"name": "datasource", "label": "Data source", "type": "datasource", "query": "prometheus", "refresh": 1,
         "current": {}},
        var("job", 'label_values(net_sessions{job!="dummy"}, job)', {"text": "gameserver", "value": "gameserver"}),
        var("instance", 'label_values(net_sessions{job=~"$job"}, instance)', ALL),
        var("group", 'label_values(group_sessions{job=~"$job", instance=~"$instance"}, group)', ALL),
    ]},
    "panels": panels,
}
out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "dashboards" / "gameserver.json"
json.dump(dashboard, open(out, "w", encoding="utf-8", newline="\n"), ensure_ascii=False, indent=2)
print(len(panels), "panels")
