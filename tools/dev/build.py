import os
import re
import time

from common import ROOT, clip, resolve_any as resolve, run

DEFAULT_BUILD_DIR = "../out/build/x64-debug"
ERROR_RX = re.compile(r"(error [A-Z]+\d+|fatal error|error:|LNK\d+|FAILED:|undefined reference)", re.IGNORECASE)
WARN_RX = re.compile(r"warning [A-Z]+\d+|warning:", re.IGNORECASE)

# Windows는 build.ps1과 같은 방식으로 VS 개발자 셸에 들어가야 cl이 잡힌다.
PS_PREFIX = (
    "$ErrorActionPreference='Stop'; "
    "$vs = & \"${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe\" -latest -products '*' "
    "-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath; "
    "Import-Module \"$vs/Common7/Tools/Microsoft.VisualStudio.DevShell.dll\"; "
    "Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null; "
)


def quote(s):
    return "'" + s.replace("'", "''") + "'"


def shell(command, cwd, timeout):
    if os.name == "nt":
        return run(["powershell", "-NoProfile", "-Command", PS_PREFIX + command], cwd=cwd, timeout=timeout)
    return run(["bash", "-lc", command], cwd=cwd, timeout=timeout)


def build(build_dir=DEFAULT_BUILD_DIR, targets=(), jobs=8, max_lines=30, timeout=1800):
    full = resolve(build_dir)
    cmd = f"cmake --build {quote(full)} --parallel {jobs}"
    if targets:
        cmd += " --target " + " ".join(quote(t) for t in targets)
    start = time.time()
    code, out, err = shell(cmd, ROOT, timeout)
    text = out + "\n" + err
    lines = text.splitlines()
    errors = list(dict.fromkeys(l.strip() for l in lines if ERROR_RX.search(l)))
    warnings = len(dict.fromkeys(l.strip() for l in lines if WARN_RX.search(l)))
    result = {"ok": code == 0, "seconds": round(time.time() - start, 1), "warnings": warnings,
              "errors": errors[:max_lines]}
    if code != 0 and not errors:
        result["tail"] = clip("\n".join(lines[-max_lines:]), 3000)
    return result


def test(build_dir=DEFAULT_BUILD_DIR, executable="", gtest_filter="", ctest_regex="", timeout=900):
    """executable이 있으면 gtest 실행 파일을 직접, 없으면 ctest를 돌린다."""
    start = time.time()
    if executable:
        exe = resolve(executable)
        args = quote(exe) + (f" --gtest_filter={quote(gtest_filter)}" if gtest_filter else "")
        code, out, err = shell("& " + args if os.name == "nt" else args, os.path.dirname(exe), timeout)
    else:
        args = f"ctest --test-dir {quote(resolve(build_dir))} --output-on-failure"
        if ctest_regex:
            args += f" -R {quote(ctest_regex)}"
        code, out, err = shell(args, ROOT, timeout)
    text = out + "\n" + err
    lines = text.splitlines()
    failed = [l.strip() for l in lines if re.search(r"\[  FAILED  \]|\*\*\*Failed|Failed\s*$", l)]
    summary = [l.strip() for l in lines if re.search(r"\[  (PASSED|FAILED)  \] \d+|tests passed|tests failed", l)]
    result = {"ok": code == 0, "seconds": round(time.time() - start, 1), "summary": summary,
              "failed": list(dict.fromkeys(failed))[:30]}
    if code != 0:
        result["tail"] = clip("\n".join(lines[-40:]), 4000)
    return result


TOOLS = [
    {
        "name": "build",
        "description": "cmake --build를 VS 개발자 셸에서 실행하고 성공 여부·소요 시간·에러 줄(중복 제거)·경고 수만 반환한다. 로그 전체는 돌려주지 않는다.",
        "schema": {
            "type": "object",
            "properties": {
                "build_dir": {"type": "string", "default": DEFAULT_BUILD_DIR},
                "targets": {"type": "array", "items": {"type": "string"}},
                "jobs": {"type": "integer", "default": 8},
                "max_lines": {"type": "integer", "default": 30},
            },
        },
        "fn": build,
    },
    {
        "name": "test",
        "description": "gtest 실행 파일(executable, gtest_filter)을 직접 돌리거나 ctest를 실행하고 통과/실패 요약과 실패한 케이스만 반환한다.",
        "schema": {
            "type": "object",
            "properties": {
                "build_dir": {"type": "string", "default": DEFAULT_BUILD_DIR},
                "executable": {"type": "string", "description": "gtest 실행 파일 경로. 비우면 ctest"},
                "gtest_filter": {"type": "string"},
                "ctest_regex": {"type": "string"},
            },
        },
        "fn": test,
    },
]
