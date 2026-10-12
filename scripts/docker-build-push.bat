@echo off
setlocal EnableExtensions DisableDelayedExpansion
chcp 65001 >nul

rem 사용법: scripts\docker-build-push.bat dev 또는 release
rem Windows의 CMD나 PowerShell에서 실행합니다.
rem GHCR 인증 정보는 Docker에 저장하며, 토큰을 이 파일에 넣지 않습니다.
rem 테스트 실행과 서비스 배포는 아직 포함하지 않습니다.
set "TASK_MODE=%~1"
if "%TASK_MODE%"=="" set "TASK_MODE=dev"
if /i "%TASK_MODE%"=="--help" goto :help
if /i "%TASK_MODE%"=="/?" goto :help
if /i "%TASK_MODE%"=="dev" goto :dev
if /i "%TASK_MODE%"=="release" goto :release
echo [오류] dev 또는 release를 지정하세요.
exit /b 2

:dev
set "TASK_MODE=dev"
set "TASK_PRESET=linux-Debug"
goto :start

:release
set "TASK_MODE=release"
set "TASK_PRESET=linux-Release"
goto :start

:start
rem 실행한 터미널의 현재 경로와 관계없이 저장소 루트를 사용합니다.
pushd "%~dp0.."
if errorlevel 1 exit /b 1
set "TASK_IMAGE=ghcr.io/pshpjr/gameserver"
set "TASK_BUILDER=gameserver-ghcr"
set "TASK_SEED=toolchain"

where docker >nul 2>nul
if errorlevel 1 (
    echo [오류] Docker 명령을 찾을 수 없습니다.
    goto :failed
)
rem Docker의 LF 줄바꿈을 findstr /x가 잘못 판정할 수 있어 출력값을 직접 읽습니다.
set "TASK_DOCKER_OS="
for /f "delims=" %%I in ('docker info --format "{{.OSType}}"') do set "TASK_DOCKER_OS=%%I"
if not defined TASK_DOCKER_OS (
    echo [오류] Docker 엔진 정보를 조회하지 못했습니다. 위 연결 오류를 확인하세요.
    goto :failed
)
if /i not "%TASK_DOCKER_OS%"=="linux" (
    echo [오류] 현재 Docker 엔진은 %TASK_DOCKER_OS%입니다. Linux 컨테이너 모드를 사용하세요.
    goto :failed
)
docker buildx version
if errorlevel 1 goto :failed

rem 기존 서브모듈 작업 내용을 바꾸지 않고, 필요한 소스 존재 여부만 확인합니다.
if not exist "NetLib\CMakeLists.txt" goto :missing_submodules
if not exist "Dummy\CMakeLists.txt" goto :missing_submodules
if not exist "src\Common\FVector.cpp" goto :missing_submodules

rem 저장된 로그인 정보를 사용합니다. 없으면 사용자 이름과 토큰을 입력합니다.
echo [1/4] GHCR 인증
docker login ghcr.io
if errorlevel 1 goto :failed

rem 레지스트리 캐시를 지원하는 전용 builder를 만들거나 재사용합니다.
rem --use를 지정하지 않아 다른 작업의 기본 builder 설정을 바꾸지 않습니다.
docker buildx inspect "%TASK_BUILDER%" >nul 2>nul
if errorlevel 1 (
    docker buildx create --name "%TASK_BUILDER%" --driver docker-container
    if errorlevel 1 goto :failed
)
docker buildx inspect "%TASK_BUILDER%" --bootstrap
if errorlevel 1 goto :failed

echo [2/4] 이전 바이너리 캐시 조회
docker buildx imagetools inspect "%TASK_IMAGE%:vcpkg-cache" >nul 2>nul
if errorlevel 1 (
    echo 이전 바이너리 캐시가 없거나 조회할 수 없어 빈 캐시로 시작합니다.
) else (
    set "TASK_SEED=%TASK_IMAGE%:vcpkg-cache"
)

rem dev와 release의 레이어 캐시는 서로 덮어쓰지 않고, 양쪽 캐시를 읽습니다.
rem 기반 이미지와 의존성 파일이 일치하는 단계만 BuildKit이 재사용합니다.
echo [3/4] %TASK_PRESET% 빌드 및 이미지 업로드
docker buildx build ^
    --builder "%TASK_BUILDER%" ^
    --platform linux/amd64 ^
    --target build ^
    --build-arg "BUILD_PRESET=%TASK_PRESET%" ^
    --build-arg "VCPKG_CACHE_IMAGE=%TASK_SEED%" ^
    --cache-from "type=registry,ref=%TASK_IMAGE%:buildcache-dev" ^
    --cache-from "type=registry,ref=%TASK_IMAGE%:buildcache-release" ^
    --cache-to "type=registry,ref=%TASK_IMAGE%:buildcache-%TASK_MODE%,mode=max" ^
    --tag "%TASK_IMAGE%:%TASK_MODE%" ^
    --push ^
    .
if errorlevel 1 goto :failed

rem 같은 builder와 같은 입력을 사용해 방금 설치한 바이너리 캐시를 내보냅니다.
rem 최초 빌드도 처리하며, 빌드 실패 시에는 이 단계에 도달하지 않습니다.
echo [4/4] vcpkg 바이너리 캐시 업로드
docker buildx build ^
    --builder "%TASK_BUILDER%" ^
    --platform linux/amd64 ^
    --target binary-cache ^
    --build-arg "VCPKG_CACHE_IMAGE=%TASK_SEED%" ^
    --tag "%TASK_IMAGE%:vcpkg-cache" ^
    --push ^
    .
if errorlevel 1 goto :failed

echo [완료] %TASK_IMAGE%:%TASK_MODE%
echo [완료] %TASK_IMAGE%:buildcache-%TASK_MODE%
echo [완료] %TASK_IMAGE%:vcpkg-cache
popd
exit /b 0

:missing_submodules
echo [오류] 서브모듈 소스가 없습니다. 저장소 루트에서 다음 명령을 실행하세요.
echo git submodule update --init --recursive
goto :failed

:failed
echo [실패] 위 오류를 확인하세요. 남은 업로드 단계는 실행하지 않습니다.
popd
exit /b 1

:help
echo 사용법: scripts\docker-build-push.bat [dev^|release]
echo dev: linux-Debug, release: linux-Release. 기본값은 dev입니다.
echo GHCR에 빌드 이미지, 레이어 캐시, vcpkg 바이너리 캐시를 업로드합니다.
exit /b 0
