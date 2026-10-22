# syntax=docker/dockerfile:1

# 빌드 환경과 산출물을 포함하는 이미지입니다. 테스트는 아직 연결하지 않았습니다.
# Windows에서 scripts\docker-build-push.bat dev 또는 release를 실행합니다.
# 서브모듈이 초기화된 상태에서 스크립트가 Buildx로 빌드, 캐시 복원, 업로드를 처리합니다.
# 새 머신에서도 GHCR의 캐시를 가져오며, 캐시가 없거나 조회할 수 없을 때만 빈 캐시로 시작합니다.
# 레이어 캐시와 vcpkg 바이너리 캐시는 각각 별도로 저장합니다.
# 다운로드와 저장 완료된 바이너리는 같은 builder의 캐시 마운트에도 남깁니다.
ARG VCPKG_CACHE_IMAGE=toolchain

FROM ubuntu:26.04 AS toolchain

ARG VCPKG_REF=19780d9cdf84d0944cf9a318666703b89ab6629c
ARG BUILD_JOBS=2

ENV VCPKG_ROOT=/opt/vcpkg \
    VCPKG_INSTALLED_DIR=/opt/vcpkg_installed \
    VCPKG_BINARY_SOURCES="clear;files,/opt/vcpkg-binary-cache,readwrite" \
    VCPKG_MAX_CONCURRENCY=${BUILD_JOBS} \
    CMAKE_BUILD_PARALLEL_LEVEL=${BUILD_JOBS} \
    CC=gcc-14 \
    CXX=g++-14

# MySQL 빌드에 필요한 RPC 헤더와 libedit의 터미널 함수 선언을 함께 설치합니다.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        bison \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        flex \
        g++-14 \
        gcc-14 \
        git \
        libncurses-dev \
        libtirpc-dev \
        libtool \
        ninja-build \
        pkg-config \
        python3 \
        tar \
        unzip \
        zip \
    && rm -rf /var/lib/apt/lists/*

# vcpkg 버전은 프로젝트의 의존성 레지스트리 baseline 커밋에 맞춰 고정합니다.
RUN git init "${VCPKG_ROOT}" \
    && git -C "${VCPKG_ROOT}" remote add origin https://github.com/microsoft/vcpkg.git \
    && git -C "${VCPKG_ROOT}" fetch --depth=1 origin "${VCPKG_REF}" \
    && git -C "${VCPKG_ROOT}" checkout --detach FETCH_HEAD \
    && "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics

# 기본은 빈 캐시입니다. VCPKG_CACHE_IMAGE로 지정한 이미지에서 캐시를 가져올 수 있습니다.
RUN mkdir -p /opt/vcpkg-binary-cache
FROM ${VCPKG_CACHE_IMAGE} AS binary-cache-seed

FROM toolchain AS dependencies
# ON이면 OTLP exporter 의존성(vcpkg otel 기능)을 함께 설치하고 NetLib을 NETLIB_ENABLE_OTEL로 빌드합니다.
ARG NETLIB_ENABLE_OTEL=OFF
# ON이면 Tracy 클라이언트(vcpkg tracy 기능, on-demand)를 함께 설치하고 NetLib을 NETLIB_ENABLE_TRACY로 빌드합니다.
ARG NETLIB_ENABLE_TRACY=OFF
WORKDIR /workspace/GameServer
# tee로 진행 로그를 표시하면서 vcpkg의 실패 코드를 유지합니다.
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# GHCR로 내보낼 수 있도록 바이너리 캐시를 일반 이미지 레이어에 저장합니다.
COPY --from=binary-cache-seed /opt/vcpkg-binary-cache/ /opt/vcpkg-binary-cache/

# 소스만 바뀌면 이 레이어를 재사용합니다. 두 Linux 프리셋 모두 x64-linux를 사용합니다.
COPY vcpkg.json vcpkg-configuration.json ./
# 캐시 마운트는 실패하거나 취소된 설치에서도 다운로드와 저장 완료된 바이너리를 보존합니다.
# GHCR에서 가져온 바이너리를 먼저 합치고, 성공하면 마운트의 내용을 이미지 레이어로 복사합니다.
# 공유 캐시의 동시 쓰기를 막기 위해 sharing=locked를 사용합니다.
# 실패한 컨테이너 레이어의 로그는 직접 열기 어려우므로 원인 로그를 출력합니다.
RUN --mount=type=cache,id=gameserver-vcpkg-downloads-x64-linux,target=/opt/vcpkg/downloads,sharing=locked \
    --mount=type=cache,id=gameserver-vcpkg-binaries-x64-linux,target=/var/cache/vcpkg-binaries,sharing=locked \
    set -eu; \
    cp -a /opt/vcpkg-binary-cache/. /var/cache/vcpkg-binaries/; \
    task_install_status=0; \
    task_feature_args=(); \
    if [ "${NETLIB_ENABLE_OTEL}" = ON ]; then task_feature_args+=(--x-feature=otel); fi; \
    if [ "${NETLIB_ENABLE_TRACY}" = ON ]; then task_feature_args+=(--x-feature=tracy); fi; \
    VCPKG_BINARY_SOURCES="clear;files,/var/cache/vcpkg-binaries,readwrite" \
    "${VCPKG_ROOT}/vcpkg" install \
        --triplet=x64-linux \
        "${task_feature_args[@]}" \
        --x-install-root="${VCPKG_INSTALLED_DIR}" 2>&1 | tee /tmp/vcpkg-install.log || task_install_status=$?; \
    if [ "${task_install_status}" -ne 0 ]; then \
        task_log_list="$(grep -oE '/opt/vcpkg/buildtrees/[^[:space:]]+\.log' /tmp/vcpkg-install.log | sort -u || true)"; \
        for task_log in ${task_log_list}; do \
            if [ -f "${task_log}" ]; then \
                printf '\n===== %s =====\n' "${task_log}"; \
                tail -n 200 "${task_log}"; \
            fi; \
        done; \
        exit "${task_install_status}"; \
    fi; \
    cp -a /var/cache/vcpkg-binaries/. /opt/vcpkg-binary-cache/; \
    rm -f /tmp/vcpkg-install.log; \
    rm -rf "${VCPKG_ROOT}/buildtrees" "${VCPKG_ROOT}/packages"

# 의존성 목록과 소스는 제외하고 바이너리 캐시만 별도 이미지로 내보냅니다.
# vcpkg는 패키지별 ABI 해시를 확인해 호환되는 바이너리만 복원합니다.
FROM scratch AS binary-cache
COPY --from=dependencies /opt/vcpkg-binary-cache/ /opt/vcpkg-binary-cache/

FROM dependencies AS build
ARG BUILD_PRESET=linux-Release
ARG NETLIB_ENABLE_OTEL=OFF
ARG NETLIB_ENABLE_TRACY=OFF

COPY . .

# Docker가 소스를 복사하기 전에 호스트에서 서브모듈을 초기화해야 합니다.
RUN test -f NetLib/CMakeLists.txt \
    && test -f Dummy/CMakeLists.txt \
    && test -f src/Common/FVector.cpp \
    && case "${BUILD_PRESET}" in \
        linux-Debug|linux-Release) ;; \
        *) echo "BUILD_PRESET must be linux-Debug or linux-Release" >&2; exit 1 ;; \
    esac \
    && cmake --preset "${BUILD_PRESET}" \
        -DCMAKE_C_COMPILER="${CC}" \
        -DCMAKE_CXX_COMPILER="${CXX}" \
        -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
        -DVCPKG_INSTALLED_DIR="${VCPKG_INSTALLED_DIR}" \
        -DNETLIB_ENABLE_OTEL="${NETLIB_ENABLE_OTEL}" \
        -DNETLIB_ENABLE_TRACY="${NETLIB_ENABLE_TRACY}" \
    && cmake --build --preset "${BUILD_PRESET}" \
    && ln -s "/workspace/out/build/${BUILD_PRESET}/GameServer" /opt/gameserver

# 덤프 설정과 마운트가 준비된 경우에만 컨테이너 명령을 실행합니다.
COPY --chmod=755 scripts/docker-entrypoint.sh /usr/local/bin/docker-entrypoint
ENTRYPOINT ["/usr/local/bin/docker-entrypoint"]

# 산출물 경로: /workspace/out/build/<BUILD_PRESET>
CMD ["/bin/bash"]

# 실행 이미지는 빌드 도구와 소스 없이 실행 파일과 런타임 의존성만 포함한다.
FROM ubuntu:26.04 AS runtime
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates libstdc++6 libgcc-s1 locales \
    && localedef -i ko_KR -f UTF-8 ko_KR.UTF-8 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /runtime

FROM runtime AS local-gameserver
ARG BUILD_PRESET=linux-Release
COPY --from=build /workspace/out/build/${BUILD_PRESET}/GameServer/CMakeServer /opt/gameserver/CMakeServer
COPY --from=build /workspace/GameServer/*.toml /runtime/
CMD ["/opt/gameserver/CMakeServer"]

FROM runtime AS local-dummy
ARG BUILD_PRESET=linux-Release
COPY --from=build /workspace/out/build/${BUILD_PRESET}/Dummy/Dummy /opt/dummy
COPY --from=build /workspace/GameServer/Dummy/DummySetting.toml /workspace/GameServer/Dummy/chatData.txt /runtime/
CMD ["/opt/dummy"]
