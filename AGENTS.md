# 저장소 작업 규칙

## 임시 자료 (scratch)

- 실제 서비스 코드에 포함하지 않을 디버깅·성능 측정·검증 스크립트와 결과물은 `scratch/<YYYY-MM-DD>_<이름>/`에 둔다. `build/`에는 두지 않는다.
- 경험 카드·조사 보고서 같은 기록은 사용자가 요청할 때만 만든다. 요청이 없으면 결과는 답변으로 전달한다.
- 작업이 끝나면 이번에 만든 임시 자료를 정리한다. 경험 카드 작성이나 scratch 정리가 필요할 때만 [scratch/AGENTS.md](scratch/AGENTS.md)를 읽는다.

## 메트릭·관측

- 계측 항목, Prometheus 이름·라벨, Grafana 대시보드, 확인 방법은 [docs/metrics.md](docs/metrics.md)에 있다. 성능·부하·동작 확인이 필요하면 로그보다 먼저 Prometheus HTTP API로 조회한다.
- 메트릭 이름은 코드와 중복이므로 문서에서 찾지 말고 `curl -s http://127.0.0.1:9090/api/v1/label/__name__/values`로 조회한다.
- 대시보드 JSON은 직접 고치지 않고 `deploy/local/config/grafana/make_dashboard.py`를 고친 뒤 다시 생성한다.
- 메트릭을 추가·변경하면 docs/metrics.md의 표도 함께 고친다.

## 개발 도구 (tools/dev)

반복되는 셸 호출은 `python -I tools/dev/dev.py <도구> '<JSON 인자>'` 한 번으로 묶는다. 도구 목록과 인자는 `python -I tools/dev/dev.py list`로 확인한다. 자세한 설명은 [tools/dev/README.md](tools/dev/README.md).

- `replace_in_file`: 파일 치환. BOM·CRLF를 보존하고 여러 편집을 원자적으로 적용한다. 치환용 파이썬 스니펫을 새로 쓰지 않는다.
- `grep_context`: 정규식 여러 개와 줄 범위(`ranges`)를 한 번에 읽는다. 여러 번 `grep`·`sed -n`을 잇지 않는다.
- `cluster_status`: 원격 서버 세 대의 상태와 비정상 컨테이너를 병렬로 요약한다.
- `prom_query`: PromQL 여러 개를 한 번에 질의한다. `window_seconds`로 구간 통계를 낸다.
- `repo_snapshot`: git 브랜치·최근 커밋·변경 파일과 scratch 최근 항목.
- `build`, `test`: VS 개발자 셸에서 빌드·테스트하고 에러와 실패 케이스만 반환한다. `build`는 Windows OTel 활성화 서버·더미·에코 서버로 검증했다. `test`는 실제 테스트 실행 파일로 검증하지 않았다.

## 환경 (Windows)

- 빌드: VS 개발자 환경이 필요하다. `cmake`·`ninja`는 PATH에 없으므로 `vcvars64.bat`를 먼저 호출한다. 프리셋의 출력 위치는 저장소 상위의 `out/build/<프리셋>`이다. `VCPKG_ROOT`는 `C:\vcpkg`.
  `cmd /c "call \"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat\" >nul 2>&1 & cmake --build ..\out\build\x64-Debug"`
- Git Bash에서 컨테이너 안 경로를 인자로 주는 `docker exec`·`docker cp`는 `MSYS_NO_PATHCONV=1`을 앞에 붙인다. 없으면 `/docker-entrypoint-initdb.d/…`가 Windows 경로로 바뀐다.
- 파이썬은 `python -I`로 실행한다. 작업 폴더의 같은 이름 모듈이 import되는 것을 막는다.
- 로컬 통합 환경(`deploy/local`): Grafana `http://localhost:3000`(admin/admin), Prometheus `http://127.0.0.1:9090`. 컨테이너 제어는 `deploy/local/local.ps1`.
- 원격 서버 세 대(`deploy/servers`, 계정 `pshpjr`, `deploy.ps1`로 제어):

  | 역할 | 주소 | 내용 |
  |---|---|---|
  | infra | 192.168.1.1 | MySQL, OTel Collector, Prometheus(9090), Grafana(3001) |
  | gameserver | 192.168.1.5 | 게임 서버(11333) |
  | dummy | 192.168.1.3 | 더미 2개 |

## 줄바꿈

- 줄바꿈 규칙은 `.gitattributes`와 `.editorconfig`가 정한다. C++ 소스·헤더는 CRLF(UTF-8 BOM 유지), 스크립트·YAML·Dockerfile·Markdown은 LF다. 파일을 고칠 때 기존 BOM과 개행을 바꾸지 않는다.
