import fnmatch
import os
import re

from common import ROOT, decode, resolve

SKIP_DIRS = {".git", "out", "build", "vcpkg_installed", "node_modules", "__pycache__", ".vs"}


def iter_files(paths, globs):
    for p in paths:
        full = resolve(p)
        if os.path.isfile(full):
            yield full
            continue
        for dirpath, dirs, files in os.walk(full):
            dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
            for name in files:
                if not globs or any(fnmatch.fnmatch(name, g) for g in globs):
                    yield os.path.join(dirpath, name)


def rel(full):
    return os.path.relpath(full, ROOT).replace("\\", "/")


def read_lines(full):
    return decode(open(full, "rb").read()).splitlines()


def grep_context(patterns=(), paths=(".",), globs=(), context=2, max_matches=20, ignore_case=False, ranges=()):
    """patterns 여러 개를 한 번에 찾고, ranges로 파일의 줄 범위도 함께 읽는다."""
    out = []
    flags = re.IGNORECASE if ignore_case else 0
    compiled = [(p, re.compile(p, flags)) for p in patterns]
    hits = {p: 0 for p in patterns}
    for full in iter_files(paths, globs):
        try:
            lines = read_lines(full)
        except OSError:
            continue
        for pat, rx in compiled:
            for i, line in enumerate(lines):
                if hits[pat] >= max_matches:
                    break
                if not rx.search(line):
                    continue
                hits[pat] += 1
                lo, hi = max(0, i - context), min(len(lines), i + context + 1)
                body = "\n".join(f"{n + 1}{':' if n == i else '-'} {lines[n]}" for n in range(lo, hi))
                out.append(f"## {rel(full)} [{pat}]\n{body}")
    for r in ranges:
        lines = read_lines(resolve(r["path"]))
        start, end = max(1, r.get("start", 1)), min(len(lines), r.get("end", r.get("start", 1) + 40))
        body = "\n".join(f"{n}: {lines[n - 1]}" for n in range(start, end + 1))
        out.append(f"## {r['path']} {start}-{end} (전체 {len(lines)}줄)\n{body}")
    summary = ", ".join(f"{p}={n}" for p, n in hits.items())
    return (summary + "\n\n" if summary else "") + "\n\n".join(out)


TOOLS = [{
    "name": "grep_context",
    "description": "여러 정규식을 여러 경로에서 한 번에 찾아 앞뒤 문맥과 함께 반환한다. ranges로 파일의 줄 범위(sed -n 대용)도 같이 읽는다. out/build/.git은 건너뛴다.",
    "schema": {
        "type": "object",
        "properties": {
            "patterns": {"type": "array", "items": {"type": "string"}},
            "paths": {"type": "array", "items": {"type": "string"}, "default": ["."]},
            "globs": {"type": "array", "items": {"type": "string"}, "description": "예: [\"*.cpp\", \"*.h\"]"},
            "context": {"type": "integer", "default": 2},
            "max_matches": {"type": "integer", "default": 20, "description": "패턴별 최대 일치 수"},
            "ignore_case": {"type": "boolean", "default": False},
            "ranges": {"type": "array", "items": {
                "type": "object",
                "properties": {"path": {"type": "string"}, "start": {"type": "integer"}, "end": {"type": "integer"}},
                "required": ["path"],
            }},
        },
    },
    "fn": grep_context,
}]
