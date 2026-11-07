import json
import time
import urllib.parse
import urllib.request

DEFAULT_URL = "http://127.0.0.1:9090"


def fetch(url):
    with urllib.request.urlopen(url, timeout=20) as r:
        return json.load(r)


def label_text(metric):
    items = {k: v for k, v in metric.items() if k != "__name__"}
    return ",".join(f"{k}={v}" for k, v in sorted(items.items())) or "-"


def fmt(v):
    f = float(v)
    return f"{f:.6g}"


def one(base, name, expr, window, step):
    if window:
        end = time.time()
        q = urllib.parse.urlencode({"query": expr, "start": end - window, "end": end, "step": step})
        data = fetch(f"{base}/api/v1/query_range?{q}")
    else:
        data = fetch(f"{base}/api/v1/query?{urllib.parse.urlencode({'query': expr})}")
    if data.get("status") != "success":
        return {"name": name, "error": data.get("error", "unknown")}
    rows = []
    for s in data["data"]["result"]:
        if window:
            vals = [float(v) for _, v in s["values"] if v not in ("NaN", "+Inf", "-Inf")]
            if not vals:
                continue
            rows.append({"labels": label_text(s["metric"]), "last": fmt(vals[-1]),
                         "min": fmt(min(vals)), "avg": fmt(sum(vals) / len(vals)), "max": fmt(max(vals))})
        else:
            rows.append({"labels": label_text(s["metric"]), "value": fmt(s["value"][1])})
    return {"name": name, "expr": expr, "series": rows}


def prom_query(queries, window_seconds=0, step=15, base_url=DEFAULT_URL):
    """queries: 문자열 또는 {name, expr}. window_seconds>0이면 구간 통계(last/min/avg/max)."""
    out = []
    for q in queries:
        name, expr = (q["name"], q["expr"]) if isinstance(q, dict) else (q, q)
        try:
            out.append(one(base_url.rstrip("/"), name, expr, window_seconds, step))
        except Exception as e:
            out.append({"name": name, "error": str(e)})
    return out


TOOLS = [{
    "name": "prom_query",
    "description": "Prometheus HTTP API(기본 http://127.0.0.1:9090)에 여러 PromQL을 한 번에 질의해 시리즈별 값을 반환한다. window_seconds>0이면 최근 구간의 last/min/avg/max로 요약한다.",
    "schema": {
        "type": "object",
        "properties": {
            "queries": {"type": "array", "items": {"anyOf": [
                {"type": "string"},
                {"type": "object", "properties": {"name": {"type": "string"}, "expr": {"type": "string"}}, "required": ["name", "expr"]},
            ]}},
            "window_seconds": {"type": "integer", "default": 0},
            "step": {"type": "integer", "default": 15},
            "base_url": {"type": "string", "default": DEFAULT_URL},
        },
        "required": ["queries"],
    },
    "fn": prom_query,
}]
