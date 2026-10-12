#!/bin/bash
set -eu

# 직접 docker run을 사용해도 덤프 설정 없이 실행되지 않도록 검사합니다.
if ! ulimit -c unlimited; then
    echo '[오류] core 제한을 해제할 수 없습니다. --ulimit core=-1로 실행하세요.' >&2
    exit 1
fi
if ! grep -q ' /dumps ' /proc/self/mountinfo || [ ! -w /dumps ]; then
    echo '[오류] 쓰기 가능한 호스트 덤프 디렉터리를 /dumps에 마운트하세요.' >&2
    exit 1
fi
if [ "$(cat /proc/sys/kernel/core_pattern)" != '/dumps/core.%e.%P.%t' ]; then
    echo '[오류] Docker 호스트의 core_pattern이 덤프 경로와 일치하지 않습니다.' >&2
    echo 'Linux Docker 호스트에서 sudo bash scripts/configure-core-dumps.sh를 실행하세요.' >&2
    exit 1
fi

# 서버에 종료 신호가 직접 전달되도록 현재 프로세스를 교체합니다.
exec "$@"
