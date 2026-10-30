from concurrent.futures import ThreadPoolExecutor

from common import clip, run

DEFAULT_HOSTS = ["pshpjr@192.168.1.1", "pshpjr@192.168.1.3", "pshpjr@192.168.1.5"]
BAD_STATUS = ("Exited", "Restarting", "Dead", "unhealthy", "Created")

REMOTE = (
    "hostname; echo ---up; uptime; echo ---mem; free -m | sed -n 2p; "
    "echo ---disk; df -h / | tail -1; echo ---docker; "
    "docker ps -a --format '{{.Names}}|{{.Status}}'"
)


def check_host(host, extra, log_lines, name_filter):
    cmd = REMOTE + (f"; echo ---extra; {extra}" if extra else "")
    try:
        code, out, err = run(["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", host, cmd], timeout=40)
    except Exception as e:
        return {"host": host, "reachable": False, "error": str(e)}
    if code != 0 and not out:
        return {"host": host, "reachable": False, "error": clip(err, 300)}
    sections, cur = {}, None
    for line in out.splitlines():
        if line.startswith("---"):
            cur = line[3:]
            sections[cur] = []
        elif cur is None:
            sections.setdefault("name", []).append(line)
        else:
            sections[cur].append(line)
    containers = [c.split("|", 1) for c in sections.get("docker", []) if "|" in c and name_filter in c.split("|", 1)[0]]
    bad = [f"{n}: {s}" for n, s in containers if any(b in s for b in BAD_STATUS)]
    res = {
        "host": host,
        "reachable": True,
        "name": " ".join(sections.get("name", [])),
        "uptime": " ".join(sections.get("up", [])).strip(),
        "mem(MB total/used/free)": " ".join(sections.get("mem", [])),
        "disk": " ".join(sections.get("disk", [])),
        "containers": f"{len(containers) - len(bad)}/{len(containers)} 정상",
        "problems": bad,
    }
    if log_lines:
        for n, s in containers:
            if n in bad_names(bad):
                _, logs, _ = run(["ssh", "-o", "BatchMode=yes", host, f"docker logs --tail {log_lines} {n} 2>&1"], timeout=30)
                res.setdefault("logs", {})[n] = clip(logs, 1500)
    if "extra" in sections:
        res["extra"] = clip("\n".join(sections["extra"]), 2000)
    return res


def bad_names(bad):
    return {b.split(":", 1)[0] for b in bad}


def cluster_status(hosts=(), extra_command="", log_lines=0, container_filter="portfolio26"):
    targets = list(hosts) or DEFAULT_HOSTS
    with ThreadPoolExecutor(max_workers=len(targets)) as ex:
        results = list(ex.map(lambda h: check_host(h, extra_command, log_lines, container_filter), targets))
    return results


TOOLS = [{
    "name": "cluster_status",
    "description": "원격 호스트들(기본 192.168.1.1/.3/.5)에 ssh로 병렬 접속해 uptime·메모리·디스크·docker 컨테이너 상태를 요약한다. 비정상 컨테이너만 problems에 모으고, log_lines>0이면 그 컨테이너의 로그 꼬리도 붙인다.",
    "schema": {
        "type": "object",
        "properties": {
            "hosts": {"type": "array", "items": {"type": "string"}, "description": "user@host 목록. 비우면 기본 3대"},
            "extra_command": {"type": "string", "description": "호스트마다 추가로 실행할 읽기 전용 명령"},
            "log_lines": {"type": "integer", "default": 0},
            "container_filter": {"type": "string", "default": "portfolio26", "description": "이름에 이 문자열이 든 컨테이너만 판정. 빈 문자열이면 전부"},
        },
    },
    "fn": cluster_status,
}]
