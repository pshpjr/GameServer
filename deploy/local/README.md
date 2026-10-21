# 로컬 통합 실행

`./local.ps1 build` 후 `./local.ps1 create`로 컨테이너를 준비한다.
MySQL이 healthy 상태가 된 뒤 서버와 더미를 각각 시작한다.
설정은 `.env`, `mysql.env`, `config/`에서 읽으며 기존 데이터 볼륨을 자동 삭제하지 않는다.
