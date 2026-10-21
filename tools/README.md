# tools

성능 측정과 Tracy 트레이스 분석에 쓰는 도구 모음이다.

## Tracy 공식 실행 파일 (Git 제외)

아래 파일은 용량이 크고(파일당 14~33MB) 버전을 올릴 때마다 저장소가 커지므로 커밋하지 않는다. `.gitignore`가 `/tools/tracy-*.exe`를 제외한다.

| 파일 | 용도 |
|---|---|
| `tracy-capture.exe` | 실행 중인 서버에 붙어 `.tracy` 파일로 저장 |
| `tracy-capture-daemon.exe` | 캡처를 데몬 형태로 실행 |
| `tracy-csvexport.exe` | 트레이스의 zone 통계와 이벤트를 CSV로 내보내기 |
| `tracy-profiler.exe` | 트레이스를 보는 GUI |
| `tracy-update.exe` | 트레이스 파일 압축 방식 갱신 |
| `tracy-merge.exe` | 트레이스 병합 |
| `tracy-import-chrome.exe`, `tracy-import-fuchsia.exe` | 다른 형식의 트레이스 가져오기 |

현재 로컬에 있는 버전은 **0.14.1** (`30997d5`)이다. 서버에 연결하는 Tracy 클라이언트와 버전이 맞아야 하므로 프로젝트의 vcpkg Tracy 버전과 같은 것을 쓴다.

받는 방법:

- Tracy 저장소(`wolfpld/tracy`)의 릴리스에서 같은 버전의 Windows 배포 파일을 받아 이 폴더에 푼다.
- 또는 vcpkg의 Tracy 0.14.1 소스로 직접 빌드한다.

라이선스는 Tracy 저장소의 `LICENSE`를 따른다(BSD 3-clause로 알고 있다. 확인이 필요하면 그 파일을 읽는다). 이 파일들을 저장소 밖으로 배포하게 되면 라이선스 문구를 함께 넣는다.

## tracy-crop (소스만 커밋)

트레이스에서 지정한 시간 구간만 잘라 새 `.tracy` 파일로 저장한다. 사용법과 제한은 [tracy-crop/USAGE.md](tracy-crop/USAGE.md)를 본다.

`tracy-crop.exe`와 `zstd.dll`은 커밋하지 않는다. 소스에서 만든다.

```powershell
.\GameServer\tools\tracy-crop\build.ps1
```
