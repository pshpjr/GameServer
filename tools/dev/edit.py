import os

from common import resolve


def replace_one(path, old, new, count):
    full = resolve(path)
    raw = open(full, "rb").read()
    bom = raw.startswith(b"\xef\xbb\xbf")
    text = raw.decode("utf-8-sig")
    crlf = "\r\n" in text
    if crlf:
        old = old.replace("\r\n", "\n").replace("\n", "\r\n")
        new = new.replace("\r\n", "\n").replace("\n", "\r\n")
    found = text.count(old)
    if found == 0:
        raise ValueError(f"NOT FOUND in {path}")
    if count == 1 and found > 1:
        raise ValueError(f"{found}곳이 일치함 (count=0이면 전부, 아니면 문맥을 늘릴 것): {path}")
    text = text.replace(old, new) if count == 0 else text.replace(old, new, count)
    tmp = full + ".tmp"
    with open(tmp, "wb") as f:
        f.write((b"\xef\xbb\xbf" if bom else b"") + text.encode("utf-8"))
    os.replace(tmp, full)
    return found if count == 0 else min(count, found)


def replace_in_file(edits):
    """edits 전체를 검증한 뒤 적용한다. 하나라도 실패하면 이미 바꾼 파일도 되돌린다."""
    backups = {}
    results = []
    try:
        for e in edits:
            full = resolve(e["path"])
            if full not in backups:
                backups[full] = open(full, "rb").read()
            n = replace_one(e["path"], e["old"], e["new"], e.get("count", 1))
            results.append(f"{e['path']}: {n}곳 치환")
    except Exception:
        for full, data in backups.items():
            with open(full, "wb") as f:
                f.write(data)
        raise
    return "\n".join(results)


TOOLS = [{
    "name": "replace_in_file",
    "description": "파일의 문자열을 치환한다. UTF-8 BOM과 CRLF를 보존하고, 여러 편집을 한 번에 받아 하나라도 실패하면 전부 되돌린다. count=1(기본)은 일치가 유일해야 하고 0이면 전부 치환한다.",
    "schema": {
        "type": "object",
        "properties": {"edits": {"type": "array", "items": {
            "type": "object",
            "properties": {
                "path": {"type": "string"},
                "old": {"type": "string"},
                "new": {"type": "string"},
                "count": {"type": "integer", "default": 1},
            },
            "required": ["path", "old", "new"],
        }}},
        "required": ["edits"],
    },
    "fn": replace_in_file,
}]
