"""반복 작업을 한 번의 셸 호출로 묶는 개발 도구 모음.

사용:
  python -I tools/dev/dev.py list
  python -I tools/dev/dev.py <도구> '<JSON 인자>'
  python -I tools/dev/dev.py <도구> -        # JSON을 표준입력으로
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import build
import cluster
import edit
import prom
import repo
import search

TOOLS = {}
for module in (edit, search, cluster, prom, repo, build):
    for tool in module.TOOLS:
        TOOLS[tool["name"]] = tool


def main(argv):
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stdin.reconfigure(encoding="utf-8")
    if len(argv) < 2 or argv[1] in ("-h", "--help"):
        print(__doc__)
        return 2
    if argv[1] == "list":
        for t in TOOLS.values():
            print(f"{t['name']}: {t['description']}")
            print("  " + json.dumps(t["schema"].get("properties", {}), ensure_ascii=False))
        return 0
    tool = TOOLS.get(argv[1])
    if tool is None:
        print(f"알 수 없는 도구: {argv[1]} (list로 확인)", file=sys.stderr)
        return 2
    raw = argv[2] if len(argv) > 2 else "{}"
    args = json.loads(sys.stdin.read() if raw == "-" else raw)
    out = tool["fn"](**args)
    print(out if isinstance(out, str) else json.dumps(out, ensure_ascii=False, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
