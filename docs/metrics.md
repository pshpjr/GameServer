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

### 게임 (게임 서버)

| Prometheus 이름 | 종류 | 의미 |
|---|---|---|
| `game_packets_total{type="move"\|"attack"\|"chat"}` | 카운터 | 필드가 처리한 요청 패킷 ([Field.cpp](../GameServer/Field.cpp)) |

