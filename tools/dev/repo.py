import os

from common import ROOT, clip, resolve, run


def git(repo, *args):
    return run(["git", "-C", repo, *args], timeout=30)[1].strip()


def repo_snapshot(repos=(".", "NetLib"), log_count=5, scratch_dir="scratch"):
    out = []
    for r in repos:
        full = resolve(r)
        if git(full, "rev-parse", "--is-inside-work-tree") != "true":
            out.append(f"## {r}: git 저장소 아님")
            continue
        out.append(
            f"## {r}\n브랜치: {git(full, 'branch', '--show-current') or '(detached)'}\n"
            f"최근 커밋:\n{git(full, 'log', f'--oneline', f'-{log_count}')}\n"
            f"변경:\n{clip(git(full, 'status', '--short'), 2000) or '(깨끗함)'}"
        )
    sdir = os.path.join(ROOT, scratch_dir)
    if os.path.isdir(sdir):
        rows = []
        for name in sorted(os.listdir(sdir), key=lambda n: os.path.getmtime(os.path.join(sdir, n)), reverse=True)[:15]:
            p = os.path.join(sdir, name)
            files = size = 0
            for dp, _, fs in os.walk(p):
                for f in fs:
                    files += 1
                    size += os.path.getsize(os.path.join(dp, f))
            rows.append(f"{name}  {files}개 {size / 1024:.0f}KB" if os.path.isdir(p) else f"{name}  {os.path.getsize(p) / 1024:.0f}KB")
        out.append(f"## {scratch_dir} (최근 수정순 15개)\n" + "\n".join(rows))
    return "\n\n".join(out)


TOOLS = [{
    "name": "repo_snapshot",
    "description": "여러 git 저장소의 브랜치·최근 커밋·변경 파일과 scratch 폴더의 최근 항목(파일 수·크기)을 한 번에 반환한다.",
    "schema": {
        "type": "object",
        "properties": {
            "repos": {"type": "array", "items": {"type": "string"}, "default": [".", "NetLib"]},
            "log_count": {"type": "integer", "default": 5},
            "scratch_dir": {"type": "string", "default": "scratch"},
        },
    },
    "fn": repo_snapshot,
}]
