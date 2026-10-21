# Executable 변경 전·후 성능 비교

2026-09-16, AdGuard 완전 종료 후 동일 Go 에코 클라이언트 004로 NetLib `792bd0a` → `679c517`을 비교했다. 테스트한 조건에서 성능 저하는 관찰되지 않았으며 약 0.1% 차이는 반복 범위가 겹쳐 개선으로 해석하지 않는다.

| 세션 수 | 버전별 반복 | 변경 전 pkt/s | 변경 후 pkt/s | 평균 묶음 RTT ms, 전 → 후 |
|---:|---:|---:|---:|---:|
| 64 | 3 | 5,082,241 | 5,088,300 | 0.403 → 0.402 |
| 512 | 4 | 4,942,384 | 4,947,164 | 3.315 → 3.312 |
| 1024 | 1 | 4,899,114 | 4,903,794 | 6.688 → 6.682 |

16회 모두 오류 0건, 종료 코드 0이었다. 1024세션은 보조 단회 검사다. [실행별 측정값](runs.csv)에 비교 판단에 사용한 16개 결과를 남긴다.

## 조건과 해석

Windows 11 Pro, Ryzen 7 9800X3D(8코어/16논리 CPU), MSVC 19.51.36257.0, Ninja, x64 Release(`/O2 /Ob2 /DNDEBUG`), Tracy OFF. 에코 서버 소스는 상위 저장소 `95ddb7df4407e9a77b73932aa8c4b095213946e9`, IOCP worker/concurrency는 각각 4였다.

TCP loopback `127.0.0.1:26001`, batch 32, delay 0, timeout 3s, reconnect=false, GOMAXPROCS 기본값 16. 매 실행마다 서버·클라이언트를 새로 시작하고 서버 준비 후 1초 대기했다. 클라이언트를 22초 실행해 첫 5개 보고 구간을 제외하고 6~20번째 약 1초 구간을 집계했다. 실행별 평균 rx의 중앙값을 비교했으며 RTT는 rx로 가중한 구간 평균의 실행별 중앙값이다. p95UpperMsMean은 히스토그램 구간 상한의 평균으로 정확한 전체 p95가 아니다. CPU 표본은 약 500ms 간격으로 수집하고 16논리 CPU로 정규화해 5~20초 구간을 평균했다.

처음에는 구버전까지 낮은 처리량과 높은 지연을 보였다. 사용자가 AdGuard 필터링 영향을 확인했고 완전 종료 후 같은 바이너리로 약 500만 pkt/s와 64세션 RTT 약 0.40ms가 회복됐다. 필터링이 남아 있던 측정은 제외했다. AdGuard를 다시 켜는 역방향 실험은 하지 않았다.

결과는 같은 PC의 짧은 loopback 실행에 한정된다. 원격 NIC, 다른 패킷 크기, Group/커스텀 콜백 및 장시간 재접속은 이 비교의 검증 범위가 아니다.

## 측정에 사용한 Go 에코 클라이언트

TCP 에코 부하와 응답 내용을 검증하는 클라이언트다. 소스는 [client/main.go](client/main.go)와 [client/main_test.go](client/main_test.go)(기존 **003 버전**)이며 외부 패키지와 go.mod는 필요 없다. Go 1.23 이상, 저장소 루트 기준.

```powershell
go run ./docs/experience-cards/2026-09-16_executable-performance/client/main.go -ip 127.0.0.1 -port 6000 -sessions 128 -batch 32 -delay 1ms -duration 30s
go test ./docs/experience-cards/2026-09-16_executable-performance/client/main.go ./docs/experience-cards/2026-09-16_executable-performance/client/main_test.go
```

인자 없이 실행하면 ip, port, sessions, reconnect, delay, batch를 차례로 묻는다. **이 소스의 기본 세션 수는 1**이다. 세션·batch 범위는 각각 1~4096, 기본 포트 6000, batch 32, delay 1ms다. `-timeout 3s`, `-duration 0`(무제한), `-reconnect`, `-reconnect-every 10s`를 지원한다.

세션당 워커 하나가 묶음 송신 → 전체 응답 검증 → 대기 순서로 동작한다. 패킷은 little-endian uint16 길이 8과 uint64 증가값으로 총 10바이트다. 재접속은 현재 묶음을 확인한 뒤 연결을 닫고 1초 후 수행한다. 오류가 있으면 종료 코드 1, 잘못된 옵션은 2다.

RTT는 묶음 전체의 응답 시간이다. p95는 히스토그램 구간 상한이고, 시간 제한 종료 시 진행 중인 묶음이 취소되어 송신·검증 수가 다를 수 있다. client-scheduler는 Go 고루틴의 실행 가능 상태 대기시간이다.

2026-09-26 정리 후 검증: 보존한 소스·테스트로 `go test -count=1` 통과.

### 클라이언트 조사에서 남길 판단

- 001의 0시간 통계는 처리율 분모 0과 히스토그램의 unsigned 언더플로 문제였다. 002에서 0시간 출력 생략과 최소 히스토그램 구간으로 수정했고 현재 테스트에도 경계값 검증이 있다.
- 003은 초기 연결을 동시에 시도한다. 당시 64·256세션은 통과했으나 1024세션에서 545개 연결 성공, 479개 connection refused를 관찰했다. 입력 상한 4096을 안정적으로 연결했다는 뜻은 아니다.
- batch 4096·2세션에서는 실제 서버가 연결을 종료했다. 독립 에코 검증은 통과했으나 서버 종료 원인은 미확인이다.

### 004 실행 파일

[artifacts/echo-client-004.exe](artifacts/echo-client-004.exe)는 위 비교와 재접속 측정에 사용한 원본이다(Git 제외, 로컬 보관). **004 소스와 테스트는 현재 파일 및 확인한 Git 이력에 없으므로 `client/main.go`로 이 바이너리를 재빌드할 수 있다고 간주하지 않는다.** 003 소스를 빌드한 파일로 대체하면 당시와 동일한 클라이언트 조건이 아니다. 같은 이름으로 덮어쓰지 않는다.

SHA-256: `832f14993b6a94ca8bbccb1c215862c6341a372ff1066cfb49f5a759c97cdc9d`.
기존 기록에는 004가 초기 연결을 약 1ms 간격으로 늘린다고 적혀 있다. 이는 보존한 003 소스의 동작과 구분한다.

## 과거 조건의 재현

서버는 위 상위 저장소 커밋과 각 NetLib 커밋을 조합해 같은 옵션으로 다시 빌드한다.

```powershell
# 해당 버전으로 빌드한 서버를 별도 터미널에서 실행
./NetLibTest_Echo.exe 26001
# 저장소 루트 기준; sessions는 64, 512, 1024로 변경
./docs/experience-cards/2026-09-16_executable-performance/artifacts/echo-client-004.exe -ip 127.0.0.1 -port 26001 -sessions 512 -batch 32 -delay 0 -duration 22s -timeout 3s
```

## 별도 재접속 실행

2026-09-16 에코 재접속: x64 Debug·loopback·128세션·batch 32·delay 1ms·10초마다 재접속으로 600.328초 실행했다. 연결 성공 7,040회, 오류 0건, 검증 패킷 1,231,218,144개였고 서버 생존을 확인했다. 송수신 중 강제 종료와 종료 후 세션 반환 완료는 검증하지 않았다. 당시 root `9500ac9`, NetLib `c129c31`에 미커밋 변경이 포함된 실행으로 현재 코드의 검증 결과가 아니다.

```powershell
./docs/experience-cards/2026-09-16_executable-performance/artifacts/echo-client-004.exe -ip 127.0.0.1 -port 6000 -sessions 128 -batch 32 -delay 1ms -reconnect -reconnect-every 10s -duration 10m -timeout 3s
```

중복 보고서·집계 JSON·정상 반복 로그·원시 자료 압축본과 재빌드 가능한 서버 실행 파일, 버전별 소스 복사본·설치 도구·빌드 캐시는 남기지 않는다.
