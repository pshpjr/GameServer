# Go 에코 클라이언트

TCP 에코 부하와 응답 내용을 검증하는 클라이언트다. 보존한 소스는 기존 **003 버전**의 [main.go](main.go)와 [main_test.go](main_test.go)다. 외부 패키지와 go.mod는 필요 없다.

## 실행과 검증

Go 1.23 이상에서 저장소 루트를 기준으로 실행한다.

```powershell
go run ./scratch/go-echo-client_2026-09-15/main.go -ip 127.0.0.1 -port 6000 -sessions 128 -batch 32 -delay 1ms -duration 30s
go test ./scratch/go-echo-client_2026-09-15/main.go ./scratch/go-echo-client_2026-09-15/main_test.go
```

인자 없이 실행하면 ip, port, sessions, reconnect, delay, batch를 차례로 묻는다. **이 소스의 기본 세션 수는 1**이다. 세션·batch 범위는 각각 1~4096, 기본 포트 6000, batch 32, delay 1ms다. `-timeout 3s`, `-duration 0`(무제한), `-reconnect`, `-reconnect-every 10s`를 지원한다.

세션당 워커 하나가 묶음 송신 → 전체 응답 검증 → 대기 순서로 동작한다. 패킷은 little-endian uint16 길이 8과 uint64 증가값으로 총 10바이트다. 재접속은 현재 묶음을 확인한 뒤 연결을 닫고 1초 후 수행한다. 오류가 있으면 종료 코드 1, 잘못된 옵션은 2다.

RTT는 묶음 전체의 응답 시간이다. p95는 히스토그램 구간 상한이고, 시간 제한 종료 시 진행 중인 묶음이 취소되어 송신·검증 수가 다를 수 있다. client-scheduler는 Go 고루틴의 실행 가능 상태 대기시간이다.

2026-09-26 정리 후 검증: 보존한 소스·테스트로 `go test -count=1` 통과.

## 조사에서 남길 판단

- 001의 0시간 통계는 처리율 분모 0과 히스토그램의 unsigned 언더플로 문제였다. 002에서 0시간 출력 생략과 최소 히스토그램 구간으로 수정했고 현재 테스트에도 경계값 검증이 있다.
- 003은 초기 연결을 동시에 시도한다. 당시 64·256세션은 통과했으나 1024세션에서 545개 연결 성공, 479개 connection refused를 관찰했다. 입력 상한 4096을 안정적으로 연결했다는 뜻은 아니다.
- batch 4096·2세션에서는 실제 서버가 연결을 종료했다. 독립 에코 검증은 통과했으나 서버 종료 원인은 미확인이다.

## 과거 측정에 사용한 004 실행 파일

[artifacts/echo-client-004.exe](artifacts/echo-client-004.exe)는 2026-09-16 재접속·성능 측정에 사용한 원본이다. **004 소스와 테스트는 현재 파일 및 확인한 Git 이력에 없으므로 main.go로 이 바이너리를 재빌드할 수 있다고 간주하지 않는다.** 같은 이름으로 덮어쓰지 않는다.

```powershell
./scratch/go-echo-client_2026-09-15/artifacts/echo-client-004.exe -ip 127.0.0.1 -port 6000 -sessions 128 -batch 32 -delay 1ms -reconnect -reconnect-every 10s -duration 10m -timeout 3s
```

SHA-256: `832f14993b6a94ca8bbccb1c215862c6341a372ff1066cfb49f5a759c97cdc9d`.
기존 기록에는 004가 초기 연결을 약 1ms 간격으로 늘린다고 적혀 있다. 이는 보존한 003 소스의 동작과 구분한다.

2026-09-16 에코 재접속: x64 Debug·loopback·128세션·batch 32·delay 1ms·10초마다 재접속으로 600.328초 실행했다. 연결 성공 7,040회, 오류 0건, 검증 패킷 1,231,218,144개였고 서버 생존을 확인했다. 송수신 중 강제 종료와 종료 후 세션 반환 완료는 검증하지 않았다. 당시 root `9500ac9`, NetLib `c129c31`에 미커밋 변경이 포함된 실행으로 현재 코드의 검증 결과가 아니다.

성능 비교 조건과 결과는 [기존 비교 기록](../executable-performance_2026-09-16/README.md)에 있다. 재현에 필요한 004 바이너리 한 개만 로컬에 보존하고 버전별 소스 복사본·설치 도구·빌드 캐시·반복 로그는 남기지 않는다.
