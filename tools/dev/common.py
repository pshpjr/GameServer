import os
import subprocess

ROOT = os.path.realpath(os.environ.get("DEVTOOLS_ROOT", os.getcwd()))


def resolve(path):
    """작업 루트 기준 경로로 바꾸고 루트 밖이면 거부한다."""
    full = os.path.realpath(path if os.path.isabs(path) else os.path.join(ROOT, path))
    if os.path.commonpath([full, ROOT]) != ROOT:
        raise ValueError(f"작업 루트 밖 경로: {path}")
    return full


def resolve_any(path):
    """빌드 산출물(../out/...)처럼 루트 밖이 정상인 경로용. 읽기·실행 전용으로만 쓴다."""
    return os.path.realpath(path if os.path.isabs(path) else os.path.join(ROOT, path))


def decode(data):
    for enc in ("utf-8", "cp949"):
        try:
            return data.decode(enc)
        except UnicodeDecodeError:
            pass
    return data.decode("utf-8", errors="replace")


def run(args, cwd=None, timeout=120, shell=False):
    p = subprocess.run(args, cwd=cwd or ROOT, capture_output=True, timeout=timeout, shell=shell)
    return p.returncode, decode(p.stdout), decode(p.stderr)


def clip(text, limit=6000):
    if len(text) <= limit:
        return text
    half = limit // 2
    return text[:half] + f"\n... ({len(text) - limit}자 생략) ...\n" + text[-half:]
