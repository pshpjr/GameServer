# NetLibTest_Echo 연결 종료 진단

## 최신 결과: 사용자 AdGuard 종료 후 대조

사용자가 AdGuard를 종료한 뒤 코드와 입력을 유지해 재검사했다.

| 검사 | 종료 전 | 종료 후 |
|---|---|---|
| 새 연결의 첫 값 1200, 패킷 1개 | 58바이트 HTTP/2 문자열 수신, 실패 | 원본 10바이트 수신, 3/3회 성공 |
| 기존 입력, 16연결 × 200패킷 | 매번 클라이언트 6 실패 | 16/16연결 × 3회 성공, 총 9600개 에코 |
| 원시 TCP 값 1200 | 송수신 바이트 불일치 | 원본과 일치 |
| 독립 Python 기준 서버 | 연결 초기화 재현 | 16연결 × 3회 통과, 오염·잘린 응답 검출도 통과 |

실제 종료 후 로그:

```text
transferred=10 buffered_before=0 buffered_total=10 hex=0800b004000000000000
 event=recv session=1 index=0 value=1200
```

AdGuard 실행 상태를 바꾼 대조에서 현상이 사라졌다. 이번 실패는 NetLib의 16연결
처리 문제보다 AdGuard 활성 상태의 통신 개입에 기인한다는 근거가 확보됐다.
AdGuard 내부의 정확한 프로토콜 판별/필터 코드까지 규명한 것은 아니다.
이전의 원인 미확정 및 Python 버전 관련 관찰은 아래에 조사 이력으로 남긴다.

로그: `build/echo-diagnostics/adguard-off-single-1200/`,
`adguard-off-16/`, `adguard-off-raw.jsonl`, `adguard-off-client-validation/`.


## 결론

이 환경에서 최초 요청 값이 1200인 LAN 패킷은 TCP 수신 측에 원본과 다른
바이트열로 도착한다. NetLibTest_Echo는 변형된 첫 두 바이트 `00 00`을 길이 0으로
해석하고, 8바이트 본문 규격에 맞지 않아 연결을 종료한다.

동일 현상이 NetLib을 사용하지 않는 Python 소켓과 .NET TcpClient/TcpListener에서도
재현됐다. 현재 증거로 NetLib의 동시성 결함이나 Python 버전 결함이라고 판단할 수 없다.
16개 연결 자체가 재현의 필수 조건은 아니다.

로컬 AdGuard 및 AdguardSvc 프로세스가 실행 중이지만, 바이트열을 변형하는 주체로
확정하지 않았다. 보안 프로그램과 네트워크 설정은 변경하지 않았다.

## 클라이언트 검증

`tests/echo_probe.py --client-unit`은 실제 네트워크를 제외하고 다음을 검증한다.

- 요청 헤더와 uint64 바이트열을 독립적인 `int.to_bytes` 결과와 비교한다.
- recv가 1~3바이트씩만 반환해도 정확하게 조립한다.
- 본문 오염, 중간 EOF, 잘못된 헤더를 실패로 판정한다.
- 초기 검사처럼 전체 응답을 모아 비교하는 방식과 프레임별 수신 방식 모두 검사한다.

독립적인 Python TCP 기준 서버도 추가했으나 이 환경에서는 간헐적인 연결 초기화가
발생했다. Python 3.11에서도 재현되므로 앞선 일시적 버전 차이는 원인 근거로 채택하지
않았다. 기준 서버의 네트워크 검사는 통과했다고 보고하지 않는다.

## 최소 재현 및 실제 바이트

한 연결에서 패킷 하나만 보낸다. 송신 직전 메모리와 수신 바이트를 비교한다.

| 값 | 송신 | 수신 |
|---|---|---|
| 1199 | `0800af04000000000000` | 동일 |
| 1200 | `0800b004000000000000` | 아래의 다른 바이트열 |
| 1201 | `0800b104000000000000` | 동일 |

1200을 보낸 경우의 실제 수신:

```text
0000080700000000000000000000000000
505249202a20485454502f322e300d0a0d0a534d0d0a0d0a
0000080700000000000000000000000000
```

가운데에는 `PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n` 문자열이 있다.
Python과 .NET에서 동일한 결과를 확인했다.

## 에코 서버 대조 결과

- 원래 입력: 연결 i가 `i * 200`부터 200개 정수를 전송한다.
- 원래 입력 및 전체 응답 수신 방식으로 16개 연결 검사를 5회 실행했다.
- 매번 15/16 연결이 통과했고 **클라이언트 6번(첫 값 1200)** 이 실패했다.
- 클라이언트 6번 데이터만 단일 연결로 실행해도 3/3회 실패했다.
- 서버 로그는 해당 세션의 `connect → invalid_length(value=0) → disconnect`를 기록했다.
- 상위 비트에 클라이언트 번호를 넣은 대조 데이터에서는 16개 연결 검사가 통과했다.
  이는 원래 입력의 실패가 해결됐다는 의미가 아니다.

대표 서버 로그 (`legacy-trace/server-0.log`):

```text
[EchoTrace] ... event=connect session=1407374883553286 index=5 value=0 error=0
[EchoTrace] ... event=invalid_length session=1407374883553286 index=5 value=0 error=0
[EchoTrace] ... event=disconnect session=1407374883553286 index=5 value=0 error=-1
```

대표 클라이언트 로그:

```text
client=6 sent=2000 error=EOFError('expected=2000 received=0')
```

`error=-1`은 기존 NetLib이 전달한 값이며 OS 종료 사유가 확인됐다는 뜻은 아니다.
`send_api_true` 역시 SendPacket 반환값이며 실제 송신 완료를 뜻하지 않는다.

## 재실행

저장소 루트에서 실행한다. 6000 포트 사용자 서버와 분리하려고 진단 포트는 6001을 쓴다.

```powershell
python tests/echo_probe.py --client-unit --output build/echo-diagnostics/unit
python tests/echo_transport_probe.py --output build/echo-diagnostics/raw-python.jsonl
python tests/echo_probe.py --server out/build/verified-Debug/NetLibTest_Echo/NetLibTest_Echo.exe --legacy --unsynchronized --trace --rounds 5 --output build/echo-diagnostics/legacy-trace
python tests/echo_probe.py --server out/build/verified-Debug/NetLibTest_Echo/NetLibTest_Echo.exe --legacy --clients 1 --client-offset 6 --trace --rounds 3 --output build/echo-diagnostics/minimal-1200
```

원시 바이트 불일치 및 연결 실패 검사는 이 환경에서 종료 코드 1을 반환하는 것이
현재의 재현 결과다. 실패를 재시도로 숨기거나 해당 값을 건너뛰지 않는다.

서버를 직접 실행해 로그를 켜려면:

```powershell
.\out\build\verified-Debug\NetLibTest_Echo\NetLibTest_Echo.exe 6001 --trace
```

새 클래스는 추가하지 않았다. 로그는 선택 사항이며 기본 실행에서는 기존 콘솔
모니터링만 동작한다. `[EchoTrace]`에는 시간, 스레드, 세션, 수신 값, 잘못된 길이,
송신 API 반환값 및 해제 콜백 오류값을 기록한다. 프레임별 로그는 처리 타이밍에 영향을
줄 수 있다.

## 남은 확인

바이트 변형을 발생시키는 로컬 중간 계층의 정확한 식별이 남아 있다.
동일 원시 TCP 검사를 네트워크 필터가 다른 환경에서 대조하거나, 해당 필터의
진단 자료를 확인해야 한다. 이 결과를 근거로 NetLib 수신 코드를 우회하거나
잘못된 길이 검사를 제거하지 않았다.

## 추가 확인: 실제 NetLib 서버의 파싱 전 수신 전체

`NetLib/src/RecvExecutable.cpp`의 수신 완료 처리에서 링 버퍼의 rear를 갱신한 직후,
헤더 해석·복호화·dequeue 전에 전체 유효 바이트를 복사해 기록했다.
`NETLIB_TRACE_RECV=1`이 설정된 프로세스에서만 활성화된다.

값 1200의 10바이트 패킷 하나를 보내는 검사를 3회 실행했다.
모두 서버의 실제 IOCP 수신 완료 바이트 수는 **58**, 기존 버퍼 잔여량은 **0**이었다.
세 번 모두 전체 수신 바이트 안에 `PRI * HTTP/2.0`이 존재했다.

```text
[NetLibRecvTrace] session=1 transferred=58 buffered_before=0 buffered_total=58 front=0 rear=58
hex=0000080700000000000000000000000000505249202a20485454502f322e300d0a0d0a534d0d0a0d0a0000080700000000000000000000000000
ascii=.................PRI * HTTP/2.0....SM.....................
[EchoTrace] event=invalid_length session=1 value=0
```

따라서 여기서 0은 **TCP가 0바이트를 수신했다는 의미가 아니라, 58바이트 중 첫 두
바이트 `00 00`을 LAN 헤더로 해석한 본문 길이**다. 이전 수신 데이터가 링 버퍼에
남아서 나타난 문자열도 아니다(`buffered_before=0`).

대조 값 1199와 1201은 각각 원본과 동일한 10바이트로 수신되었고 에코에 성공했다.
TCP 프로토콜 자체가 변형했다거나 특정 프로그램이 원인이라는 결론은 여전히 내리지
않는다. 확인된 것은 이 서버의 수신 API 완료 시점에 이미 해당 바이트가 들어 있다는
사실이다.

로그 위치:

- `build/echo-diagnostics/server-raw-1200/server-0.log` (동일 폴더의 1, 2도 재현)
- `build/echo-diagnostics/server-raw-1199/server-0.log`
- `build/echo-diagnostics/server-raw-1201/server-0.log`

재실행:

```powershell
$env:NETLIB_TRACE_RECV = '1'
python tests/echo_probe.py --server out/build/verified-Debug/NetLibTest_Echo/NetLibTest_Echo.exe --legacy --clients 1 --packets 1 --client-offset 1200 --trace --rounds 3 --output build/echo-diagnostics/server-raw-1200
Remove-Item Env:NETLIB_TRACE_RECV
```

## 추가 검사: 한 연결에서 값 0부터 순차 전송

`python -B tests/echo_sequence_probe.py`로 검사했다. 각 케이스는 서버를 새로 실행하고
TCP 연결 하나만 생성했다. 패킷 하나를 보낸 뒤 동일한 10바이트 에코를 전부 확인하고
다음 값을 보냈다. 진단 포트는 6001이다.

| 값 범위 | 전송/에코 개수 | 서버 연결 수 | HTTP/2 문자열 | 결과 |
|---|---:|---:|---|---|
| 0~1200 (양 끝 포함) | 1201 / 1201 | 1 | 없음 | 통과 |
| 0~99 | 100 / 100 | 1 | 없음 | 통과 |
| 0~49 | 50 / 50 | 1 | 없음 | 통과 |

값 1200도 기존 연결의 중간 데이터로 보내면 원래 바이트 그대로 수신되고 정상 에코됐다.
따라서 값 1200이 언제나 변형된다거나 1200번째 패킷에서 발생하는 현상으로 일반화할 수
없다. 현재까지 확인한 실패 조건은 **새 연결의 첫 데이터가 값 1200인 패킷일 때**다.

결과: `build/echo-diagnostics/sequence/summary.json`

각 케이스의 클라이언트 송수신 원문과 서버 수신 원문은 같은 폴더의
`client-1201.jsonl`, `server-1201.log` 및 100/50 파일에 보존했다.
