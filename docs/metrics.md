# 메트릭·관측

게임 서버·더미의 메트릭이 어디서 기록되어 어떻게 Grafana까지 가는지, 어떤 항목이 있는지, 사람과 에이전트가 각각 어떻게 확인하는지 정리한다. 로컬 통합 환경(`deploy/local`) 기준이다. 컨테이너 실행 방법은 [deploy/local/README.md](../deploy/local/README.md)를 본다.

## 구성

```text
gameserver ─┐
dummy(들) ──┼─ OTLP/HTTP ─▶ otel-collector ─▶ Prometheus(OTLP 수신) ─▶ Grafana
            │                 ├─ mysql receiver: MySQL에 직접 접속해 상태 값을 읽는다
            │                 └─ docker_stats receiver: Docker 소켓으로 컨테이너 자원 사용량을 읽는다
```

- 서버·더미는 NetLib의 `psh::metrics`로 값을 기록하고, 내보내기 스레드가 10초마다(`Config::otlpInterval`) OTLP/HTTP로 보낸다.
- collector는 받은 것과 직접 읽은 것을 모두 Prometheus의 OTLP 수신 경로(`/api/v1/otlp`)로 밀어 넣는다. Prometheus는 아무것도 스크레이프하지 않는다.
- 내보내기는 환경 변수 `OTEL_EXPORTER_OTLP_METRICS_ENDPOINT`가 있을 때만 켜진다. compose가 gameserver·dummy·dummy2에 `http://otel-collector:4318/v1/metrics`를 넣는다. 이미지는 `.env`의 `NETLIB_ENABLE_OTEL=ON`으로 빌드되어야 한다.
- 게임 서버 콘솔의 LIBS 블록(`game.useConsoleMonitor`)도 같은 메트릭을 읽어 1초마다 출력한다(ConsoleMonitorExporter).

| 서비스 | 주소(호스트) | 용도 |
|---|---|---|
| Grafana | http://localhost:3000 | 대시보드. 첫 로그인 admin/admin |
| Prometheus | http://localhost:9090 | 저장·쿼리. HTTP API로 직접 조회할 수 있다 |
| otel-collector | http://localhost:4318/v1/metrics | OTLP 수신. Windows에서 직접 띄운 서버도 여기로 보낸다 |

## 라벨

| 라벨 | 출처 | 값 |
|---|---|---|
| `job` | OTel `service.name` | `gameserver`, `dummy`, `mysql`, `docker`, `host` |
| `instance` | OTel `service.instance.id` = 컨테이너의 `HOSTNAME` | gameserver는 `gameserver`로 고정. 더미는 컨테이너 ID라 여러 개 띄우면 서로 갈라진다 |
| `group` | NetLib 그룹 ID | 게임 서버 기준 1이 마을, 2~4가 필드(Easy·Hard·Pvp) |
| `mode` | CPU 메트릭 | `total`, `kernel`, `user` |
| `type`, `result` | 메트릭별 | 아래 표 참고 |

## 이름 규칙

코드에는 OTel 이름(점 구분)으로 등록하고, Prometheus에서는 변환된 이름으로 조회한다.

- 점은 밑줄이 된다. `net.sessions` → `net_sessions`
- Counter는 `_total`이 붙는다. `net.accepts` → `net_accepts_total`
- 단위가 이름 끝에 붙는다. 시간(`Unit::Microseconds`, 초로 변환해 내보냄) → `_seconds`, `Unit::Bytes` → `_bytes`, `Unit::Percent` → `_percent`. 단위 없음은 아무것도 붙지 않는다.
- 히스토그램은 `_bucket`(누적, `le` 라벨), `_sum`, `_count` 세 개가 된다.

## 계측 항목

### 게임 서버·더미 공통 (NetLib)

`IOCP` 생성자에서 등록한다([NormalIOCP.cpp](../NetLib/src/NormalIOCP.cpp)). 게임 서버와 더미 모두 `job`만 다르게 보낸다.

| Prometheus 이름 | 종류 | 의미 |
|---|---|---|
| `net_sessions` | 게이지(콜백) | 연결된 세션 수 |
| `net_accepts_total` | 카운터 | accept한 연결 |
| `net_disconnects_total` | 카운터 | 끊긴 세션 |
| `net_packets_recv_total`, `net_packets_sent_total` | 카운터 | 송수신 패킷. 스레드마다 100개씩 모아 더하므로 최대 99개씩 늦게 반영된다 |
| `net_timeouts_total` | 카운터 | 세션 타임아웃 |
| `net_semaphore_timeouts_total` | 카운터 | 세마포어 타임아웃으로 실패한 소켓 작업 |
| `net_sendbuffer_pool_size`, `net_sendbuffer_pool_empty` | 게이지 | 송신 버퍼 전역 풀 크기, 풀이 비어 새로 만든 누적 횟수 |
| `net_sendbuffer_acquires`, `net_sendbuffer_releases` | 게이지 | 송신 버퍼 누적 획득·반납 수. 사용 중 = acquires − releases |
| `process_cpu_usage_percent{mode}` | 게이지(소수) | 프로세스 CPU. **전체 논리 코어 합이 100%** |
| `system_cpu_usage_percent{mode}` | 게이지(소수) | 머신 전체 CPU. 같은 기준 |
| `process_memory_physical_bytes`, `process_memory_virtual_bytes` | 게이지 | 메모리. 각각 `_peak_bytes`도 있다 |

CPU·메모리·송신 버퍼 풀 값은 MonitorThread가 1초마다 `Set`한다. `docker stats`는 코어 하나가 100%라 숫자가 다르다(16코어에서 38.8% ≈ 메트릭 2.4%).

### 그룹 (NetLib, `group` 라벨)

`GroupManager::CreateGroup`이 그룹 ID를 정한 직후 `Group::InitMetrics`에서 등록한다([Group.cpp](../NetLib/src/Group.cpp)).

| Prometheus 이름 | 종류 | 의미 |
|---|---|---|
| `group_process_duration_seconds` | 히스토그램 | `Process()` 한 번 걸린 시간. 일이 없는 호출이 많아 1µs 이하가 상당수다 |
| `group_jobs_total` | 카운터 | 처리한 그룹 잡 |
| `group_enters_total`, `group_leaves_total` | 카운터 | 그룹 입장·퇴장. 레벨 이동도 포함 |
| `group_sessions` | 게이지 | 그룹 안 세션 수. 1초마다 그룹 스레드가 `Set` |
| `group_job_queue_size` | 게이지 | 쌓인 잡 수. 같은 주기 |

초당 바쁜 시간 비율은 `rate(group_process_duration_seconds_sum[1m])`이다.

### DB (게임 서버)

[DBThreadWrapper](../GameServer/DBThreadWrapper.cpp). 필드마다 DB 스레드가 하나씩 있고, 이름·속성이 같아 하나의 시리즈로 합산된다.

| Prometheus 이름 | 종류 | 의미 |
|---|---|---|
| `db_queries_total{result="ok"\|"error"}` | 카운터 | 실행한 쿼리 |
| `db_query_duration_seconds` | 히스토그램 | 쿼리 실행 시간 |
| `db_queue_wait_duration_seconds` | 히스토그램 | `Execute` 호출부터 DB 스레드가 쿼리를 시작할 때까지. 큐가 가득 차 `Enqueue`가 기다린 시간도 포함 |
| `db_queue_size` | 게이지 | 큐에 쌓인 작업 수(전체 DB 스레드 합). 스레드당 큐 용량은 1,024이고, 가득 차면 넣는 쪽 게임 스레드가 멈춘다 |

### 게임 (게임 서버)

| Prometheus 이름 | 종류 | 의미 |
|---|---|---|
| `game_packets_total{type="move"\|"attack"\|"chat"}` | 카운터 | 필드가 처리한 요청 패킷 ([Field.cpp](../GameServer/Field.cpp)) |

### 더미 (`job="dummy"`)

요청 종류마다 시리즈가 하나다([Player.cpp](../Dummy/src/Player.cpp)). `type`은 `login`, `register`, `game_login`, `level_enter`, `move`, `attack`. 채팅은 응답이 없어 빠진다.

| Prometheus 이름 | 종류 | 의미 |
|---|---|---|
| `dummy_action_delay_seconds{type}` | 히스토그램 | 요청부터 응답까지 |
| `dummy_requests_total{type}` | 카운터 | 응답을 기다리는 요청을 보낸 수 |
| `dummy_requests_pending{type}` | 게이지 | 보냈지만 아직 응답이 없는 요청 수 |
| `dummy_players{state, reason}` | 게이지 | 더미를 상태와 지금 행동하지 않는 이유로 센 수. 1초마다 그룹이 다시 센다 |
| `dummy_players_sleeping{state, reason, over="30s"\|"5m"}` | 게이지 | `reason`이 `acting`이 아닌 더미 중 그 시간 넘게 행동하지 않은 수. 30초를 넘기면 로그도 남긴다 |
| `dummy_disconnects_total{reason}` | 카운터 | 더미가 스스로 끊은 이유 |
| `dummy_leaves_total{kind}` | 카운터 | 세션이 끝난 경위 |
| `dummy_session_seconds_total` | 카운터 | 끝난 세션이 연결되어 있던 시간의 합 |
| `dummy_level_changes_total{from, to}` | 카운터 | 레벨 이동 요청. `from`은 처음 입장이면 `none` |
| `dummy_packets_recv_total{type}` | 카운터 | 서버에서 받은 패킷 수. 목록에 없는 종류는 `other` |

`state`는 `connecting`(접속 직후 로그인 전), `login`, `game_login`, `level_change`, `village`, `pve`, `pvp`, `disconnect_wait`다.

`reason`(행동하지 않는 이유)은 아래와 같다. `village`·`pve`·`pvp`에서만 `acting` 외의 값 `wait_create`, `dead`가 나온다.

| `reason` | 의미 |
|---|---|
| `acting` | 틱마다 행동을 고른다 |
| `wait_create` | 레벨 입장 뒤 내 액터 생성(`ResCreateActor`) 패킷을 기다린다. 오지 않으면 영원히 멈춘다 |
| `dead` | 서버가 알려 준 HP가 0 이하라 행동하지 않는다. 끊는 것은 Hit·Destroy 패킷이라, 그 패킷 없이 HP만 0 이하가 되면 멈춘다. 둘이 겹치면(HP 0 이하이면서 생성 패킷 대기) `dead`로 센다 |
| `wait_response` | 로그인·레벨 이동 등 응답을 기다린다 |
| `disconnecting` | 끊고 재접속을 기다린다 |

`dummy_disconnects_total`의 `reason`: `session_end`(세션 길이 만료), `death`(HP 0 이하 Hit), `destroy_dead`(HP 0 이하일 때 내 액터 삭제), `destroy_alive`(**HP가 남았는데 내 액터가 삭제됨. 서버와 상태가 어긋난 것**), `login_fail`.

`dummy_leaves_total`의 `kind`: `expected`(더미가 끊음), `reset`(예상 밖 리셋. 같은 계정이 다시 접속하면 `invalid`로 확정), `invalid`, `server_stop`. 평균 세션 길이는 `rate(dummy_session_seconds_total[1h]) / sum(rate(dummy_leaves_total[1h]))`다.

#### 더미 로그 파일

콘솔에는 시작하지 못하는 오류만 찍고, 나머지는 실행 폴더 아래 `logs/`의 파일로 남긴다(`연-월-일__종류.txt`, 종류별로 한 파일). 서버 배포는 호스트의 `~/portfolio26/data/dummyN/logs/`에 마운트된다.

| 종류 | 수준 | 내용 |
|---|---|---|
| `DummyStuck` | Error | 30초 넘게 행동하지 않은 더미 하나의 상태. 계정, state, reason, HP, `needUpdate`, 마지막으로 받은 패킷과 경과 시간, 응답 대기 중인 요청, 세션 만료 여부 |
| `DummyDisconnect` | Error·System | 예상 밖 종료(`Invalid Destroy HP > 0` 등)와 서버 중단, 종료 요약 |

### MySQL (`job="mysql"`)

collector의 mysql receiver가 10초마다 읽는다. 계정은 `monitoring.env`의 읽기 전용 계정이다. 대표 항목:

| Prometheus 이름 | 의미 |
|---|---|
| `mysql_threads{kind}` | connected·running·cached·created 스레드 |
| `mysql_query_count_total`, `mysql_query_slow_count_total` | 쿼리, 느린 쿼리 |
| `mysql_commands_total{command}` | select·insert·update·delete 등 명령별 횟수 |
| `mysql_row_operations_total{operation}` | 행 읽기·삽입·수정·삭제 |
| `mysql_row_locks_total{kind="waits"\|"time"}` | row lock 대기 횟수, 대기 시간(ms) |
| `mysql_buffer_pool_pages{kind}`, `mysql_buffer_pool_operations_total{operation}` | 버퍼 풀 페이지, 논리 읽기(`read_requests`)·디스크 읽기(`reads`) |
| `mysql_operations_total{operation}` | InnoDB fsync·read·write |
| `mysql_connection_errors_total{error}`, `mysql_max_used_connections` | 연결 에러, 최대 동시 연결 |

### 컨테이너 (`job="docker"`)

collector의 docker_stats receiver가 Docker 소켓으로 10초마다 읽는다. 컨테이너는 `container_name` 라벨로 구분한다(예: `portfolio26-local-gameserver-1`).

| Prometheus 이름 | 의미 |
|---|---|
| `container_cpu_usage_nanoseconds_total` | CPU 누적 시간. `rate(...[1m]) / 1e9`가 사용 코어 수로, `docker stats`와 같다. 같이 들어오는 `container_cpu_utilization_ratio`는 Docker Desktop에서 2~6배 크게 나와 쓰지 않는다 |
| `container_memory_usage_total_bytes` | 메모리 |
| `container_blockio_io_service_bytes_recursive_total{operation}` | 디스크 읽기·쓰기 바이트. cgroup v2라 횟수는 없다 |
| `container_network_io_usage_tx_bytes_total`, `..._rx_bytes_total` | 컨테이너가 보낸·받은 바이트(`interface="eth0"`). TCP/IP 헤더와 ACK까지 포함한 실제 전송량이다 |

**송수신 바이트는 이 컨테이너 메트릭으로 본다.** NetLib에는 바이트 카운터가 없다.

- 게임 서버 송신 = 클라이언트 하향 + DB 요청, 수신 = 클라이언트 상향 + DB 응답. DB 몫은 mysql 컨테이너의 송수신으로 뺄 수 있다.
- 더미 송신 ÷ 더미 세션 수가 클라이언트 1명의 상향 대역폭이다. OTLP 내보내기(초당 수 KB)가 섞이지만 작다.
- 2026-10-22 더미 1개(약 880명) 기준 1명당 상향 약 12kbps, 하향 약 21kbps.

### 호스트 디스크 (`job="host"`, 서버 배포 인프라만)

인프라 collector의 host_metrics receiver가 `/proc/diskstats`를 5초마다 읽는다. 컨테이너 안에서도 호스트 디스크 전체가 보이고, `sda`만 수집한다. 로컬 통합 환경에는 없다. 커널 카운터라 `iostat`·`sar -d`와 출처가 같고 주기만 다르다(`sar`은 10분 평균).

| Prometheus 이름 | 의미 |
|---|---|
| `system_disk_operations_total{direction}` | 완료한 읽기·쓰기 횟수 |
| `system_disk_operation_time_seconds_total{direction}` | 요청이 완료될 때까지 걸린 누적 시간. `rate(operation_time) / rate(operations)`가 `iostat`의 await이다 |
| `system_disk_io_time_seconds_total` | 장치가 I/O를 처리 중이던 누적 시간. `rate(...) * 100`이 `%util`이다 |
| `system_disk_weighted_io_time_seconds_total` | 처리 중 요청 수로 가중한 누적 시간. `rate(...)`가 평균 큐 길이(`aqu-sz`)이다 |
| `system_disk_pending_operations` | 순간 처리 중인 요청 수 |
| `system_disk_io_bytes_total{direction}`, `system_disk_merged_total{direction}` | 읽기·쓰기 바이트, 병합된 요청 수 |

- 쓰기 await(ms): `rate(system_disk_operation_time_seconds_total{job="host",direction="write"}[30s]) / rate(system_disk_operations_total{job="host",direction="write"}[30s]) * 1000`
- `iostat`의 `f_await`(flush 지연)는 수집되지 않는다. 쓰기 await가 대신 올라간다.
- 평상시 쓰기 await는 약 0.26ms, util은 약 6%다. 2026-10-29 약 70분마다 수십 초 동안 await 50~100ms, util 95~100%로 SSD가 멈추고, 이때 DB 쿼리와 더미의 `login`·`level_enter` 지연이 같이 튄다.
- `docker compose`로 인프라를 다시 띄우면 이 설정이 적용된다. 그 전에는 컨테이너 안에 별도 collector를 띄워 수집할 수 있다(`docker exec -d`로 띄운 프로세스는 컨테이너 재시작 시 사라진다).

## 해석할 때 주의할 점

- **지연 히스토그램 경계는 1µs~10s다**(`Histogram::LatencyUsBounds`). 10초를 넘는 값은 `histogram_quantile`이 10초로 돌려준다. 분위수는 버킷 사이를 선형 보간한 추정치이고, 평균(`_sum / _count`)은 정확하다.
- **rate는 점이 두 개 이상 있어야 나온다.** 내보내기가 10초 주기라 컨테이너를 띄운 직후 1분 정도는 `rate(...[1m])`가 비거나 일부만 나온다.
- **컨테이너를 다시 만들면 이전 instance의 시리즈가 몇 분 남는다.** 마지막 값(예: 종료 직전 세션 0)이 그대로 보이다가 사라진다.
- **`fake-gameserver`, `smoke` job은 대시보드 검증용 가짜 데이터다.** 무시한다.
- **CPU 비교는 기준을 맞춘다.** NetLib CPU는 전체 코어 합이 100%, 컨테이너 CPU와 `docker stats`는 코어 하나가 1(100%)이다. 배포 대상 192.168.1.5는 12스레드라 같은 부하라도 로컬(16스레드)과 퍼센트가 다르다.

## Grafana 대시보드

- 파일: [deploy/local/config/grafana/dashboards/gameserver.json](../deploy/local/config/grafana/dashboards/gameserver.json)
- 넣는 법: Grafana → Dashboards → New → Import로 JSON을 올린다. 위쪽 `Data source` 변수에서 Prometheus를 고른다. 같은 uid라 다시 임포트하면 덮어쓴다.
- 데이터 소스: Prometheus, 주소 `http://prometheus:9090`. 자동 등록하지 않는다.
- 변수: `job`(더미 제외, 기본 gameserver), `instance`, `group`
- 행: Overview, Network, Process, Groups, DB, Game, MySQL, Dummy(상태별 인원, 멈춘 더미, 종료 사유, 패킷 종류 포함), Containers(CPU·메모리·디스크·네트워크, 더미 세션당 대역폭)

**JSON은 직접 고치지 않는다.** [make_dashboard.py](../deploy/local/config/grafana/make_dashboard.py)를 고친 뒤 다시 생성한다.

```powershell
python deploy/local/config/grafana/make_dashboard.py
```

## 에이전트가 확인하는 법

Grafana는 비밀번호가 사용자 설정이라 API로 보지 않는다. Prometheus HTTP API로 직접 조회한다. 인증은 없고, 127.0.0.1에만 열려 있다.

```bash
# 즉시 값. 결과는 data.result[].metric / value[1]
curl -sG http://127.0.0.1:9090/api/v1/query --data-urlencode 'query=sum by (job)(net_sessions)'

# 메트릭 이름 목록
curl -sG http://127.0.0.1:9090/api/v1/label/__name__/values

# 시간 범위(최근 5분, 30초 간격)
curl -sG http://127.0.0.1:9090/api/v1/query_range --data-urlencode 'query=sum(db_queue_size)' \
  --data-urlencode "start=$(( $(date +%s) - 300 ))" --data-urlencode "end=$(date +%s)" --data-urlencode 'step=30'
```

- label·series API(`/api/v1/label/...`, `/api/v1/series`)는 `-G`로 GET을 쓴다. `--data-urlencode`만 주면 POST가 되어 label API가 거부한다.
- Git Bash에서 `docker exec`에 컨테이너 안 경로를 넘길 때는 `MSYS_NO_PATHCONV=1`을 둔다. 없으면 `/docker-entrypoint-initdb.d/...`가 Windows 경로로 바뀐다.

자주 쓰는 쿼리:

| 보고 싶은 것 | 쿼리 |
|---|---|
| 서버 CPU | `process_cpu_usage_percent{job="gameserver", mode="total"}` |
| 초당 송수신 패킷 | `sum by (job)(rate(net_packets_recv_total[1m]))` |
| 게임 요청 종류별 처리량 | `sum by (type)(rate(game_packets_total[1m]))` |
| 그룹 처리 시간 p99 | `histogram_quantile(0.99, sum by (le, group)(rate(group_process_duration_seconds_bucket{job="gameserver"}[1m])))` |
| DB 적체 | `sum(db_queue_size)`, `histogram_quantile(0.99, sum by (le)(rate(db_queue_wait_duration_seconds_bucket[1m])))` |
| 더미 응답 지연 | `histogram_quantile(0.99, sum by (le, type)(rate(dummy_action_delay_seconds_bucket[1m])))` |
| 레벨별 세션 분포 | `sum by (group)(group_sessions{job="gameserver"})` |
| MySQL 부하 | `rate(mysql_query_count_total[1m])`, `rate(mysql_operations_total{operation="fsyncs"}[1m])` |
| 컨테이너 송수신 (B/s) | `sum by (container_name)(rate(container_network_io_usage_tx_bytes_total{job="docker"}[1m]))` |
| 클라이언트 1명 상향 (bps) | `sum(rate(container_network_io_usage_tx_bytes_total{container_name=~"portfolio26-local-dummy[0-9]*-1"}[1m])) * 8 / sum(net_sessions{job="dummy"})` |

메트릭이 안 보일 때 순서:

1. 컨테이너 상태: `./local.ps1 status`. 서버 시작 로그에 `metrics disabled: ...`가 있으면 초기화가 실패한 것이다.
2. collector 오류: `./local.ps1 logs otel-collector`에서 receiver·exporter 오류를 확인한다(예: MySQL 계정 권한). 실제 수신 여부는 Prometheus의 최신 메트릭 값과 시각으로 확인한다.
3. 이미지가 OTel을 켜고 빌드됐는지: `.env`의 `NETLIB_ENABLE_OTEL=ON`. OFF로 빌드한 이미지는 콘솔 출력만 하고 OTLP로 보내지 않는다.
4. Prometheus에 이름이 있는지: 위의 `__name__` 목록.

코드를 바꾼 뒤 확인하는 순서: `./local.ps1 build <service>` → 해당 컨테이너 stop, `docker compose --env-file .env -f compose.yaml create --no-build <service>`, start → 1분 이상 기다린 뒤 rate 쿼리로 확인한다. 빌드 중에는 소스를 수정하지 않는다. 실행 중인 컨테이너가 새 이미지를 사용하는지도 확인한다.

## 메트릭 추가

- `psh::metrics::Registry::Create*`로 등록한다. 등록은 잠금을 잡으므로 생성자나 초기화에서 한 번만 하고, 루프 안에서 만들지 않는다. 기록(`Add`, `Set`, `Record`)은 잠금 없는 atomic이다.
- 같은 이름·속성으로 다시 등록하면 같은 시리즈를 돌려준다. 같은 이름은 종류·단위·경계가 같아야 하고, 다르면 빈 핸들이 나와 기록이 무시된다(stderr에 `[metrics] rejected`).
- 다른 스레드에서 읽으면 안 되는 값은 `CallbackGauge` 대신 소유 스레드가 주기적으로 `Gauge::Set`한다.
- 시간은 `CreateTimeHistogram` + `RecordSince(TimePoint::Now())`로 µs 단위로 기록한다.
- 이름은 OTel 규칙(`영역.대상[.세부]`, 소문자·점)을 따르고, 종류 구분은 속성으로 한다(예: `game.packets{type}`).
- 추가한 뒤 대시보드가 필요하면 `make_dashboard.py`에 패널을 넣고 JSON을 다시 생성한다.
