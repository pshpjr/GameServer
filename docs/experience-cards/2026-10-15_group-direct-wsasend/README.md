# 경험 카드 - 그룹 스레드가 WSASend를 직접 호출하던 구조와 송신 executor 도입 전후

## 한 줄 사실

5000세션 부하에서 그룹 큐 대기가 평균 0.75초, 최대 8.4초까지 늘어난 것을 덤프와 Tracy로 확인하고, 그룹 스레드가 `WSASend`를 직접 호출하는 송신 경로(`Session::TrySend` → `RealSend`)를 executor로 옮겼더니, 같은 스레드에서 `IOCP::SendPacket`이 걸리는 시간이 호출당 19.8µs에서 2.0µs로, 그룹 핸들러(`Field::OnRecv`)는 904µs에서 258µs로 줄었다.

## 상황

* 더미 클라이언트 약 5000명(재접속, 이동, 전투, 채팅 혼합)을 서버에 걸고 Tracy로 캡처하는 Windows 실행에서, 그룹 job이 큐에서 기다리는 시간이 이상하게 길었다.
* "전" 실행의 서버 로그(`NETLIB_LATENCY_MS=100`)에서 `stage=group_queue`는 실행 끝에 누적 804,734건, 평균 745,140µs(0.75초), 최대 8,372,464µs(8.4초)였다. 100ms 이상 걸린 job이 561,728건으로 전체의 약 70%였다.
* 같은 실행에서 100ms 이상 걸린 것으로 로그에 남은 `group_update`는 10건, `packet_handler`는 1건뿐이라 지연이 핸들러 실행 시간 자체보다 큐 대기에 몰려 있었다.
* "전" 실행은 Debug 빌드다. 덤프 스택에 디버그 CRT(`ucrtbased`)가 나온다. "후" 실행의 빌드 구성은 확인하지 못했다.

## 조사 흐름

### 1. 관찰

* 확인한 사실:
  * 그룹 큐 지연이 1초를 넘는 시점(서버 가동 64초)에 받은 덤프에서, 그룹 워커 스레드가 `Group::SendPacket → IOCP::SendPacket → Session::TrySend → Session::RealSend → Socket::Send → WSASend` 안에 있었다. 즉 그룹 스레드가 `WSASend` 시스템 호출을 직접 실행한다.
  * Tracy 구간(5분 트레이스의 2:52~3:12, 20초)에서 `IOCP::SendPacket` 평균 19.80µs 중 `Session::RealSend`가 18.52µs였다. 호출 시간의 약 93%가 `RealSend`다.
  * 같은 구간 합계: `IOCP::SendPacket` 46.5초, 핸들러 `psh::Field::OnRecv` 58.2초. 둘의 크기가 비슷하다.
* 이상하다고 본 점: 트레이스에서 `Group::Update` 한 번이 약 292.5초(실행 거의 전체) 이어진 사례가 있었다. 그 그룹은 한 번도 한가해지지 않고 계속 돌았다는 뜻으로 보이지만 원인은 확인하지 못했다.

### 2. 가설

* 의심한 원인: 핸들러(`OnRecv`) 안에서 패킷을 보낼 때마다 그룹 스레드가 `WSASend` 시스템 호출까지 직접 실행해서 핸들러가 길어지고, 그 뒤에 기다리는 job들의 대기가 늘어난다.
* 그렇게 생각한 이유: 위 스택에서 그룹 스레드가 송신 호출 안에 있었고, `SendPacket` 합계(46.5초)와 핸들러 합계(58.2초)가 비슷한 크기다. 다만 호출 관계는 이 통계만으로는 확인하지 못했다.

### 3. 시도

* 한 일: 송신을 호출한 스레드에서 직접 하지 않고 executor로 넘겼다.
  * `Session::TrySend`가 `RealSend`를 바로 호출하던 것을, 세션 참조를 올린 뒤(`postSendInc`) `_owner->PostExecutable(_sendExecutable)`로 executor에 올리는 것으로 바꿨다.
  * `_sendExecutable`의 콜백은 `RealSend()`를 실행한다. `RealSend` 안에 있던 참조 증가는 TrySend 쪽으로 옮겼다.
* 비교한 기록:
  * 전: `tracy-queue` 실행의 5분 트레이스에서 2:52~3:12 구간을 `scripts/tracy-crop`으로 잘랐다(`artifacts/server_crop_before.tracy`).
  * 후: 송신 executor를 넣은 실행의 트레이스에서 약 4:21~4:31(10초)을 잘랐다([CAS 카드의 `artifacts/server_crop.tracy`](../2026-10-13_cas-state-machine/README.md)).
  * 두 크롭에서 `tracy-csvexport`로 존별 통계를 뽑았다(`zones-before.csv`, `zones-after.csv`).
* 이 방법을 선택한 이유: 5분 전체 트레이스는 1~2GB여서 비교할 구간만 잘라 두는 편이 낫다.

### 4. 결과

존별 통계(호출당 시간, µs). 구간 길이는 전 20초, 후 10초다.

| 존 | 전 | 후 |
|---|---:|---:|
| `IOCP::SendPacket`(호출 스레드) | 19.80 | 2.03 |
| `Session::RealSend` | 18.52 | 14.28 |
| `HandleJob`(그룹 job 하나) | 1,229.6 | 261.0 |
| `Field::OnRecv` | 904.4 | 257.8 |
| `Field::RecvAttack` | 904.2 | 256.5 |
| `Field::BroadcastToPlayer` | 1,471.5 | 216.5 |

초당 호출 수와 스레드 시간(구간 길이로 나눈 값).

| 존 | 전 | 후 |
|---|---:|---:|
| `IOCP::SendPacket` 호출 수 | 약 11.7만/초 | 약 23.9만/초 |
| `IOCP::SendPacket` 스레드 시간 | 약 2,327ms/초 | 약 485ms/초 |
| `Session::RealSend` 스레드 시간 | 약 2,168ms/초 | 약 3,185ms/초 |
| `Field::OnRecv` 호출 수 | 약 3,215/초 | 약 3,125/초 |
| `Field::BroadcastToPlayer` 호출 수 | 약 1,625/초 | 약 3,185/초 |

* 확인된 것:
  * 호출 스레드가 `SendPacket`에서 쓰는 시간이 약 10분의 1로 줄었다(19.80 → 2.03µs).
  * `RealSend` 자체는 호출당 18.5 → 14.3µs로 크게 줄지 않았다. 송신 비용이 사라진 것이 아니라 executor 스레드로 옮겨 갔다. 송신 호출이 두 배로 늘었기 때문에 `RealSend`의 스레드 시간 합은 오히려 늘었다.
  * 그룹 핸들러 한 번이 걸리는 시간이 약 3.5~6.8배 줄었다(`BroadcastToPlayer` 1,471 → 216µs, `OnRecv` 904 → 258µs).
  * "후" 크롭의 `Group::Update` 최대는 240.7ms로, "전"의 292.5초짜리와 달리 길게 이어진 사례가 없었다.
* 배제된 것: 없음.
* 아직 불확실한 것:
  * 초당 `SendPacket`과 `BroadcastToPlayer` 호출 수가 "후"에서 두 배다. `OnRecv` 호출 수는 같다(약 3,200/초). 왜 한 번의 수신당 송신이 늘었는지는 확인하지 못했다. 부하가 같지 않을 가능성이 있고, 이 경우 호출당 시간 비교는 영향을 받을 수 있다(같은 값이라면 "후"가 더 무거운 송신 부하를 처리한 셈이다).
  * 그룹 큐 대기(`group_queue`)가 얼마나 줄었는지는 비교하지 못했다. "후" 실행의 서버 로그가 남아 있지 않다.
  * "전" 트레이스에서 `Group::Update`가 292초 이어진 원인.
  * 두 빌드가 송신 executor 외에 큐 구현, 계측 코드 등에서 같은지 확인하지 않았다.

### 5. 다음 판단

* 송신 호출을 executor로 옮긴 뒤 같은 부하에서 송신이 멈추고 서버 종료가 안 되는 문제가 드러났다. 원인은 송신 큐(`LockFreeFixedQueue`)의 실패한 enqueue가 남기는 빈 슬롯이었고 따로 정리했다 → [송신 큐 카드](../2026-10-15_sendq-proof/README.md).

## 해결 결과

* 적용한 조치: 송신을 executor로 이동(`Session::TrySend`). 이 변경은 NetLib 작업 트리에 있고 커밋되지 않았다.
* 적용 후 결과: 위 표. 호출 스레드 송신 시간과 그룹 핸들러 시간이 줄었다.
* 재발 여부: 이 구조에서 송신 정지가 발견돼 큐를 고쳤다(위 링크). 같은 조건으로 다시 비교한 측정은 하지 않았다.

## 한계

* 각각 한 번씩 측정했고 구간 길이도 다르다(전 20초, 후 10초).
* `tracy-crop`은 지정 구간과 겹치는 이벤트를 통째로 보존하므로 크롭에 요청 구간 밖의 이벤트가 들어갈 수 있다. 위 표는 리프 존(`SendPacket`, `RealSend`, `HandleJob`, `OnRecv` 등)만 썼고, 호출 수가 전체 실행의 평균 호출 속도와 맞는 것을 확인했다. `Group::Update`, `Executor callback` 같은 부모 존의 평균은 구간 밖 이벤트가 섞여 쓰지 않았다.
* "전"은 Debug 빌드다(덤프의 `ucrtbased`). "후"의 빌드 구성은 확인하지 못했다. 구성이 다르면 위 비교에 영향이 있고, Release에서는 비율이 다를 수 있다.

## 증거

* Tracy 크롭: `artifacts/server_crop_before.tracy`(전, 2:52~3:12, 로컬 보관·Git 제외), [CAS 카드의 `artifacts/server_crop.tracy`](../2026-10-13_cas-state-machine/README.md)(후, 약 4:21~4:31)
* 존별 통계: `zones-before.csv`, `zones-after.csv`(각 크롭의 `tracy-csvexport` 결과)
* 덤프 스택: `queue-delay-stacks.txt`(전, 그룹 스레드가 `WSASend` 안에 있는 스레드)
* 코드: NetLib `src/Session.cpp`(`TrySend`, `RealSend`), `src/Group.cpp`(`Group::SendPacket`), `src/NormalIOCP.cpp`(`IOCP::SendPacket`)
* 도구: `scripts/tracy-crop`
* 커밋: 없음
