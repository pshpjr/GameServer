# tools/dev

반복되던 Bash 연속 호출을 한 번의 셸 호출로 묶는 개발 도구다. 서버를 띄우지 않고 CLI로 호출하므로 Claude Code와 Codex가 같은 명령을 쓴다. 표준 라이브러리만 쓴다.

```
python -I tools/dev/dev.py list
python -I tools/dev/dev.py <도구> '<JSON 인자>'
```

| 도구 | 하는 일 |
|---|---|
| `replace_in_file` | 문자열 치환. BOM·CRLF 보존, 여러 편집을 원자적으로 적용(하나라도 실패하면 전부 되돌림), 작업 루트 밖은 거부 |
| `grep_context` | 정규식 여러 개를 여러 경로에서 한 번에 검색, 문맥 포함. `ranges`로 줄 범위도 함께 읽음 |
| `cluster_status` | 원격 3대에 ssh 병렬 접속, uptime·메모리·디스크·컨테이너 이상 여부 요약 |
| `prom_query` | PromQL 여러 개를 한 번에 질의, `window_seconds`로 구간 통계 |
| `repo_snapshot` | git 브랜치·최근 커밋·변경, scratch 최근 항목 |
| `build` | VS 개발자 셸에서 `cmake --build`, 에러 줄과 경고 수만 반환 |
| `test` | gtest 실행 파일 또는 ctest 실행, 실패한 케이스만 반환 |

작업 루트는 현재 디렉터리이고 `DEVTOOLS_ROOT`로 바꿀 수 있다. 상대 경로의 기준이자 `replace_in_file`의 쓰기 허용 범위다. `build`·`test`만 `../out/...` 같은 루트 밖 빌드 폴더를 읽고 실행한다.

`build`는 Windows OTel 활성화 서버·더미·에코 서버 빌드로 검증했다. `test`는 아직 실제 테스트 실행 파일로 검증하지 않았다.
