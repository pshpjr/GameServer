# 경험 카드 - 송신 큐의 실패한 enqueue가 남긴 빈 슬롯 때문에 송신이 멈추고 서버 종료가 안 되던 문제

## 한 줄 사실

부하 시험 중 서버가 종료되지 않는 문제를 계측과 덤프로 추적해, 송신 큐(`LockFreeFixedQueue`)가 가득 찬 상태에서 실패한 enqueue가 번호만 소비하고 비어 있는 슬롯을 남겨 `Dequeue`가 영구히 막힌다는 것을 확인했고, 실패해도 번호를 소비하지 않는 sequence 기반 큐로 바꾼 뒤 같은 부하에서 멈춤이 재현되지 않았다.

## 상황

* 더미 클라이언트 약 5000명(재접속, 이동, 전투, 채팅 혼합)을 서버에 걸고 종료 시간을 재는 시험에서, 서버가 종료 신호 후에도 끝나지 않았다.
* 처음 발견한 것은 Tracy 캡처를 켠 Windows 5000세션 300초 실행(`send-executor` 폴더, 삭제)이다. 부하는 끝까지 돌았고(응답 954,398건) 서버 종료가 30.7초 안에 끝나지 않았다. 그 시점의 덤프 상태는 다음과 같았다.
  * `Session::RealSend`가 `LockFreeFixedQueue::Dequeue`를 반복 호출 중이었다.
  * 송신 큐 `failedReservations` = 24, `head` = 920, `tail` = 945(`Size()` = 25).
  * head 슬롯(24번)의 `isUsed`는 false인데 뒤쪽 슬롯에는 데이터가 있었다.
* 원인을 확인하려고 계측을 넣어 다시 돌린 것이 이 폴더의 Windows 시험(`run`)이다. 부하 33초 시점에 송신이 멈췄고, 종료 신호 후 5초 안에 서버가 끝나지 않았다. 이때 최대 세션 4,940개, 응답 100,985건이었다.
* Linux에서도 서버 종료가 끝나지 않는 증상이 따로 관찰됐다(아래 "같은 원인으로 본 Linux 증상").
* 송신 큐는 세션마다 `LockFreeFixedQueue<SendBufferImpl*, 128>` 하나다. 큐가 가득 찰 정도로 송신이 밀리면 `IOCP::ProcessBuffer`가 버퍼를 해제하고 세션을 끊도록 되어 있다.

## 조사 흐름

### 1. 관찰

* 확인한 사실: 덤프 스택에서 워커 스레드가 `Session::RealSend` 안에서 `LockFreeFixedQueue::Dequeue`를 반복 호출하고 있었다. `RealSend`는 보낼 버퍼를 하나 꺼낼 때까지 루프를 도는 구조다.
* 이상하다고 본 점: 큐에는 데이터가 있는데(`Size()`가 0이 아님) `Dequeue`가 계속 false를 돌려줬다.

### 2. 가설

* 의심한 원인: 기존 `Enqueue`는 `tailIndex.fetch_add(1)`로 번호를 먼저 가져간 뒤 그 슬롯이 사용 중이면 false를 돌려준다. 이때 번호는 반납되지 않는다. 그 번호의 슬롯은 영영 채워지지 않는 빈 슬롯이 되고, `Dequeue`는 head가 그 슬롯에 오면 "비어 있음"으로 보고 false를 돌려주므로 뒤쪽 슬롯에 데이터가 있어도 꺼내지 못한다.
* 그렇게 생각한 이유: 상태를 찍어 보니 head 슬롯만 비어 있고 그 뒤 슬롯들이 차 있었다.

### 3. 시도

* 관련 폴더: `artifacts/run`
* 한 일: 큐에 예약, 발행, 소비 이력(1,024개)과 상태 스냅샷을 남기는 계측(`LatencyProbe.h`와 큐 내부 프로브)을 넣고, 부하를 건 채 송신 정지가 감지되면 덤프를 받는 `run.py`로 재현했다. 서버 종료 시점의 덤프도 받았다.
* 이 방법을 선택한 이유: 문제가 드물게 생기고 상태가 곧 바뀌므로, 정지가 일어난 순간의 큐 상태와 그 직전 이력을 함께 잡아야 했다.

### 4. 결과

* 확인된 것(`artifacts/run/evidence.json`, 판정은 `hypothesis_confirmed_with_wraparound_caveat`):
  * 같은 큐에서 티켓 1411~1538 128개가 발행돼 큐가 가득 찼다.
  * 티켓 1539의 enqueue가 실패했다(`enqueue-failed-no-reservation` 계열). 이 번호의 슬롯은 3번이었다.
  * 그 뒤 1540~1550이 발행됐다. head는 1539에서 움직이지 않았고, 차 있는 슬롯은 4~15번 12개였다.
  * 처음 관찰된 실패 시점의 다른 큐에서도 `head=530 tail=624 occupied=92`이면서 head 슬롯만 비어 있었다.
* 이어서 확인된 것: 큐가 가득 차면 세션을 끊는데(`IOCP::ProcessBuffer`), 끊은 뒤 `Session::Reset`의 비우기 루프(`Dequeue`를 반복)도 같은 빈 슬롯에서 막혔다. `Reset-after-drain` 시점에도 11개가 남아 있었고, 같은 큐를 쓰는 다음 연결(generation 3)이 head 1539, tail 1551로 그 상태를 그대로 물려받았다. 다음 연결의 `RealSend`가 같은 자리에서 막혔다.
* 배제된 것: 큐 자체가 아니라 호출하는 쪽의 문제라는 가능성. 막힌 스레드의 스택에서 `RealSend` → `Dequeue`가 확인됐고, 큐를 바꾼 뒤에는 멈춤이 사라졌다(아래). 다만 비교한 빌드에는 큐 외 변경도 섞여 있다.
* 아직 불확실한 것:
  * 빈 슬롯이 일시적으로 풀리는 경우가 있었다(`caveats`, 큐 `000001F53BA88D40`에서 관찰). 한 바퀴 돈 번호(티켓 + 128)가 같은 슬롯을 채우면 head의 `Dequeue`가 성공한다. 이때 꺼낸 데이터가 head 번호의 것이 아니라 한 바퀴 뒤 번호의 것이라 순서가 어긋날 수 있다고 추정하지만 확인하지 않았다.
  * 이벤트 이력의 순서는 계측이 완료된 순서이고 큐의 실제 선형화 순서가 아니다.
  * Reset에서 비우지 못하고 남은 버퍼가 해제되지 않아 누수됐을 가능성은 확인하지 않았다.

### 5. 다음 판단

* 가설이 맞다고 판단해 큐 설계를 바꾸기로 했다. 슬롯마다 순번(sequence)을 두어 가득 찬 경우를 번호를 예약하기 전에 판단하는 방식을 택했다. 다른 대안을 검토했는지는 기록이 없다.

## 해결 결과

* 적용한 조치: `LockFreeFixedQueue`를 슬롯별 `sequence`를 쓰는 방식으로 바꿨다.
  * `Enqueue`는 `sequence == tail`일 때만 CAS로 tail을 예약하고, `sequence < tail`(한 바퀴 돌아 가득 참)이면 번호를 소비하지 않고 false를 돌려준다.
  * `Dequeue`는 `sequence == head + 1`(발행 완료)일 때만 꺼낸다.
  * `Clear()`를 추가해 `Session::Reset`에서 head, tail, 모든 slot sequence를 초기화한다.
  * 이 변경은 NetLib 작업 트리에 있고 아직 커밋되지 않았다.
* 적용 후 결과(같은 5000명 부하, 같은 더미·서버 설정). 송신 큐 크기는 `Session::MAX_SEND_COUNT`와 같다. 수정 전 `run`은 128이었다.

| 시험 | 큐 | 결과 |
|---|---|---|
| `enqueue128` | sequence 큐, 크기 128 | 부하 56초 시점까지 큐가 가득 차 enqueue가 5회 실패했다. 송신 정지 0회, 서버 종료 0.89초 정상, 최대 세션 4,987, 응답 182,271건 |
| `retest` | sequence 큐, 크기 512 | 300초 전체, enqueue 실패 0회라서 실패 경로는 이 시험에서 실행되지 않았다. 송신 정지 0회, Reset 후 큐에 데이터 남은 경우 0회, 재접속 972회, 서버 종료 0.43초, 최대 세션 5,000, 응답 943,121건 |

* 수정 효과를 실패 상황에서 시험한 것은 크기를 128로 둔 `enqueue128`이다. 크기를 512로 키운 `retest`는 큐가 가득 차지 않아, 큐 교체 효과와 크기 증가 효과를 이 시험만으로 가를 수 없다.
* 패치 비교(`NetLib-source.patch`)상 `run`과 `retest`의 차이는 큐 교체, `Session::Reset`의 `_sendQ.Clear()` 추가, `MAX_SEND_COUNT` 128 → 512다. `enqueue128`은 128이다. 나머지 계측 코드와 송신 executor 변경은 세 빌드에 모두 들어 있다.

* 재발 여부: 이 시험 범위에서는 없었다. 단일 PC, Windows, 각 300초 이하다. Linux에서의 재검증과 장시간 시험은 하지 않았다. 현재 작업 트리의 `MAX_SEND_COUNT`는 256이라 시험한 128, 512와 다르다.

## 같은 원인으로 본 Linux 증상

* Linux(WSL)에서 서버가 종료 신호 후 30.5초 안에 끝나지 않았다. gdb 스택에서 메인 스레드가 `IOCP::Wait`의 스레드 `join()`에서 기다리고 있었다.
* Linux 5000세션 1시간 시험: 부하 구간 3,599초 동안 오류는 보고되지 않았고 세션은 5000개를 유지했으며 서버 RSS는 약 223MB에서 늘지 않았다. 종료 신호 후 서버가 60초 안에 끝나지 않았다.
* 이 둘은 위 문제와 같은 원인으로 판단했다. 다만 Linux 쪽에서 큐 상태를 직접 확인한 것은 아니고, "종료가 끝나지 않는다"는 증상과 큐 수정 후 재검증 부재를 근거로 한 판단이다. 해당 폴더의 원본 기록은 삭제했다.

## 증거

* 로그·계측: `artifacts/run/evidence.json`(빈 슬롯 확인), `artifacts/run/result.json`(정지 시점), `artifacts/enqueue128/result.json`, `artifacts/retest/result.json`
* 스택: `artifacts/run/server-send-stall-stacks.txt`(`RealSend` → `Dequeue`)
* 코드: NetLib `include/LockFreeFixedQueue.h`, `src/Session.cpp`(`Reset`, `RealSend`), `src/NormalIOCP.cpp`(`ProcessBuffer`)
* 재현 스크립트: `run.py`(수정 전 큐로 정지를 재현), `enqueue128.py`, `retest.py`
* 커밋: 없음
