# 로컬 통합 테스트

## 설정 위치

- `.env`: 이미지 태그, 빌드 종류, 호스트 공개 포트. Compose가 읽는다.
- `mysql.env`: 로컬 DB 초기 계정. 컨테이너 환경 변수로 전달한다. DB의 초기화 인터페이스이므로 파일 마운트 대신 env_file을 사용한다.
- `config/mysql.cnf`: MySQL 설정. 컨테이너에 읽기 전용 마운트한다.
- `config/server/*.toml`: 서버 설정. 컨테이너에 마운트하여 실행 시 직접 읽는다. 실제 DB 연결 설정은 GameSettings.toml이다.
- `config/dummy/DummySetting.toml`: 더미 설정. 마운트하여 읽는다. 기본 설정은 첫 더미 100명, 두 번째 더미 1,000명이며 계정 범위가 겹치지 않도록 설정한다.
- `../mysql/init/*.sql`: 초기 스키마·테스트 데이터. 초기화 경로에 읽기 전용 마운트한다.
- `config/otel-collector.yaml`: OTLP/HTTP(4318)로 받아 Prometheus의 OTLP 수신 경로로 넘긴다. debug 출력으로 수신을 확인한다. mysql receiver가 MySQL 상태 값을 10초마다 읽어 `job="mysql"`로 넘긴다.
- `monitoring.env`: mysql receiver용 읽기 전용 MySQL 계정. mysql과 otel-collector가 함께 읽는다. 계정은 `../mysql/init/03-monitoring.sh`가 빈 볼륨 초기화 때 만든다. 이미 있는 DB에는 `docker exec portfolio26-local-mysql-1 bash /docker-entrypoint-initdb.d/03-monitoring.sh`로 한 번 만든다.
- `config/prometheus.yml`: 스크레이프 대상 없이 OTLP 수신만 쓴다. service.name은 job, service.instance.id는 instance 라벨이 된다.
- `seccomp-io-uring.json`: Moby 기본 syscall 필터에 io_uring_setup·io_uring_enter·io_uring_register 허용만 추가한 로컬 프로필이다. 게임 서버·더미에 적용하며 필터 전체를 해제하지 않는다. 원본: [moby/profiles](https://github.com/moby/profiles/blob/6fe7deb1b9fb7c0397a4593480d7d22b9ee8caef/seccomp/default.json), 라이선스는 LICENSE.seccomp에 포함한다.

Windows 마운트의 파일 권한 때문에 MySQL이 cnf를 무시하지 않도록 시작 시 마운트한 설정을 내부 경로에 644 권한으로 복사하여 읽는다. 더미 이미지는 코드에서 요구하는 ko_KR.UTF-8 로케일을 포함한다. 의존성 캐시 이미지 주소는 .env에서 지정한다.

더미의 현재 IP 파서는 DNS 이름 대신 숫자 IP를 받으므로 로컬 서버 주소를 172.29.26.10으로 고정했다. 기존 네트워크와 충돌하면 compose.yaml의 subnet·ipv4_address와 더미 IP를 함께 바꾼다.

## 빌드·개별 실행

Docker Desktop의 Linux 엔진을 켠다. 이 폴더에서 실행:

```powershell
./local.ps1 build
./local.ps1 create
```

위 작업은 컨테이너를 자동 시작하지 않는다. Docker Desktop에서 mysql을 먼저 시작하고 healthy 상태를 확인한 뒤 gameserver, dummy를 각각 시작한다. 기존 이미지는 자동 삭제·백업하지 않는다. 기존 이미지 백업은 별도 작업이다.

게임 서버·더미는 실행 파일·런타임 의존성·로케일만 포함하는 실행 전용 이미지를 사용한다. 빌드 도구와 소스는 실행 이미지에 포함하지 않는다.

필요할 때 스크립트로도 개별 제어한다. 서비스 주소·비밀번호·마운트 인자는 입력하지 않는다.

```powershell
./local.ps1 start mysql
./local.ps1 start gameserver
./local.ps1 start dummy
./local.ps1 stop dummy
./local.ps1 restart gameserver
./local.ps1 status
./local.ps1 logs mysql
```

한 번에 전체 시작·종료하는 기능이나 depends_on은 두지 않는다. 설정 파일 수정 후 해당 컨테이너를 재시작한다. 이미지·포트·환경 변수·마운트를 변경했다면 해당 서비스를 stop 후 create하여 재생성한다.

종료 검증은 게임 서버를 먼저 종료한다. 더미는 예상하지 못한 ConnectionReset을 즉시 Invalid로 세지 않고, 해당 계정의 재접속 성공이면 Invalid로 확정한다. 재접속이 ConnectionRefused이면 보류 건을 ServerStop으로 분류하고 더미 프로세스 전체를 정상 종료한다. 서버 종료 확정 후 다른 그룹의 연결 정리도 Invalid에서 제외한다. 종료 요약은 더미의 `/runtime/logs/` 아래 `DummyDisconnect` 로그에서 확인한다. 정상 Stop과 크래시는 서버 종료 코드·덤프에서 따로 판단한다.

MySQL 데이터는 이름 있는 볼륨에 남는다. mysql.env와 초기 SQL은 빈 데이터 볼륨의 첫 초기화에만 적용된다. 기존 DB의 계정·데이터를 자동 변경하거나 삭제하지 않는다. 로컬 자격 증명은 배포용으로 사용하지 않는다.

Windows에서 서버·더미를 직접 실행할 때는 mysql만 시작하고 기존 localhost 설정을 사용한다. 이번 로컬 타깃은 호스트 core_pattern을 강제하지 않는다. Docker Desktop의 덤프 수집 검증은 별도 작업이다.

## 메트릭

otel-collector, prometheus, grafana는 각각 시작한다. Grafana 설정은 grafana-data 볼륨에, 수집 데이터는 prometheus-data 볼륨에 남는다. Grafana 데이터 소스와 대시보드는 자동 등록하지 않는다.

- 컨테이너 안 서버의 otlpEndpoint: `http://otel-collector:4318/v1/metrics`
- Windows에서 직접 실행한 서버: `http://localhost:4318/v1/metrics`
- Prometheus: `http://localhost:9090`, Grafana: `http://localhost:3000` (첫 로그인 admin/admin)
- Grafana의 Prometheus 데이터 소스 주소: `http://prometheus:9090`
- 대시보드: `config/grafana/dashboards/gameserver.json`을 Dashboards → New → Import로 넣는다. 데이터 소스는 대시보드 변수로 고른다. JSON은 `config/grafana/make_dashboard.py`로 생성한다.
- 계측 항목과 조회 방법: [docs/metrics.md](../../docs/metrics.md)
