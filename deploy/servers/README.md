# 서버 배포 (세 서버)

| 역할 | 주소 | 띄우는 것 |
|---|---|---|
| infra | 192.168.1.1 | MySQL, OTel Collector, Prometheus, Grafana (컨테이너 하나) |
| gameserver | 192.168.1.5 | 게임 서버 (호스트 네트워크, 11333) |
| dummy | 192.168.1.3 | 더미 2개, 각 1,000명 (계정 0~999, 1000~1999) |

주소·이미지 태그·빌드 설정은 `servers.psd1`에 있다. Grafana 대시보드는 로컬 구성의 공통 JSON을 전송 준비 단계에서 복사한다. 이미지는 레지스트리 없이 `docker save` → `scp` → `docker load`로 보낸다.

## 제어

PowerShell 7(`pwsh`)에서 이 폴더 기준으로 실행한다. 실패하면 그 자리에서 멈추고 `[단계] 역할(주소) 실패: …`를 표시한다.

```powershell
./deploy.ps1 build [-Role r]      # 이미지 빌드
./deploy.ps1 transfer [-Role r]   # 이미지(서버에 없을 때만)·compose·설정 전송
./deploy.ps1 start [-Role r]      # 인프라(healthy) → 게임 서버(11333 수신) → 더미(2개 실행) 순서
./deploy.ps1 stop [-Role r]       # 더미 → 게임 서버. 인프라는 -Role infra로만 끈다
./deploy.ps1 restart -Role r
./deploy.ps1 status
./deploy.ps1 deploy [-Role r]     # build → transfer → start
```

더미 시작 전에 DB의 `LoginState = 1` 계정을 0으로 되돌린다. 이전 실행이 비정상 종료되면 로그인 상태가 남아 다시 로그인하지 못한다.

## 서버의 파일 배치

```
~/portfolio26/
  .env                      이미지 태그, INFRA_HOST (compose 공통)
  seccomp-io-uring.json     io_uring 허용 seccomp 프로필 (deploy/local과 같음)
  infra/       compose.yaml, infra.env(비밀값), config/
  gameserver/  compose.yaml, config/(GameSettings.toml, serverSetting.toml)
  dummy/       compose.yaml, config/(DummySetting*.toml, chatData.txt)
  data/infra/  mysql, prometheus, grafana 영속 데이터 (이미지 교체·종료에도 남는다)
  data/dummy1/logs/, data/dummy2/logs/  더미 로그 파일 (컨테이너를 다시 만들어도 남는다)
```

## 비밀값

`secrets.env`(MySQL root·game·모니터 계정 비밀번호)는 처음 실행할 때 무작위로 만든다. 로컬 자격 증명은 쓰지 않는다. 이 파일과 `.stage/`(전송 준비), `images/`(보낸 이미지 파일 보관)는 Git에서 제외한다.

## 인프라 이미지

`infra/Dockerfile`은 MySQL 8.4 공식 이미지에 OTel Collector, Prometheus, Grafana의 정적 바이너리를 복사한다. 빌드 컨텍스트는 `deploy/`이고 `infra/Dockerfile.dockerignore`로 필요한 파일만 넣는다.

`infra/entrypoint.sh`가 네 서비스를 띄운다. 종료 신호는 모든 서비스에 전달하고, 하나라도 끝나면 나머지를 정상 종료한 뒤 컨테이너를 끝낸다. 다시 띄우는 것은 Docker 재시작 정책(`unless-stopped`)이 컨테이너 단위로 한다. 서비스별 재시작이 필요 없어서 별도 프로세스 관리자는 두지 않았다.

| 서비스 | 포트 |
|---|---|
| MySQL | 3306 |
| OTLP/HTTP | 4318 |
| Prometheus | 9090 |
| Grafana | 3001 (1.1의 3000은 다른 서비스가 쓴다) |

## 게임 서버·더미 이미지

루트 `Dockerfile`의 실행 전용 단계(`local-gameserver`, `local-dummy`)를 그대로 쓴다. 실행 파일·필수 라이브러리·로케일만 들어 있다. 빌드는 `linux-Release` 프리셋(RelWithDebInfo), OTel ON, Tracy OFF.

두 컨테이너는 호스트 네트워크를 쓰고 `nofile`을 65536으로 올린다. 호스트 기본값 1024로는 서버가 `AcceptError 24`(EMFILE)로 1,000명 남짓에서 멈춘다.

## 아직 하지 않은 것

clean 빌드 검사, coredump 수집기 전환, 서버 종료 순서 검증(저장 → 더미 재접속 거절 확인). 심볼 추출·이미지와 Build ID 대응 기록은 기본 배포에 포함하지 않는다. [docs/deployment-plan.md](../../docs/deployment-plan.md) 참고.
