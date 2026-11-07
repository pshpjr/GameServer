# 로컬 MySQL

실행 인자는 `../local/.env`, `../local/mysql.env`, 마운트 설정 파일로 관리한다. [로컬 통합 테스트 안내](../local/README.md)를 따른다. 수동 docker run 인자를 입력할 필요는 없다.

- Windows 직접 실행 서버는 기존 GameSettings.toml의 `127.0.0.1:3306`, `game` / `password`, DB `mydb`를 사용한다.
- 서버도 컨테이너로 실행하면 공통 Docker 네트워크에 연결하고 외부 GameSettings.toml의 GameDBIP를 `portfolio26-mysql`로 설정한다.
- 테스트 계정은 `ID_0` / `PASS_0`부터 `ID_10000` / `PASS_10000`까지이며 캐릭터는 계정당 PlayerId=0 하나다. 초기 위치는 마을이다.
- 처음에는 더미의 maxPlayer를 10, playerPerGroup을 10으로 낮춰 확인한다. 현재 기본값은 5,000명이다.
- 초기 SQL은 빈 데이터 볼륨의 첫 실행에만 적용된다. 기존 볼륨에는 자동 재적용하지 않는다. 데이터 초기화를 위해 기존 볼륨을 자동 삭제하지 않는다.
- 테스트 자격 증명과 데이터는 로컬 전용이다. 기존 서버 인증의 세션 키 검증 강화 등은 이번 DB 복구 범위에 포함하지 않는다.
- 강제 종료로 LoginState가 남으면 게임 서버와 더미를 모두 중지한 뒤 MySQL에서 `UPDATE mydb.account SET LoginState=0;`을 실행하여 테스트 상태를 정리한다.

검증 쿼리:

```sql
SELECT COUNT(*) FROM mydb.account; -- 10001
SELECT COUNT(*) FROM mydb.player;  -- 10001
SELECT * FROM mydb.player WHERE AccountNo=0 AND PlayerId=0;
```
