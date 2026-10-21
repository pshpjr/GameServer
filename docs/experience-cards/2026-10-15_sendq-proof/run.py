import argparse
import ctypes
import json
import os
import re
from pathlib import Path
import shutil
import signal
import socket
import subprocess
import sys
import time

parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--seconds', type=int, default=300)
parser.add_argument('--port', type=int, default=21333)
parser.add_argument('--label')
args = parser.parse_args()
if not 1 <= args.seconds <= 300: parser.error('seconds must be between 1 and 300')
windows = os.name == 'nt'
if args.label and not re.fullmatch(r'[a-zA-Z0-9_-]+', args.label):
    parser.error('label must contain only letters, digits, underscore or hyphen')
root = Path(__file__).resolve().parent / 'artifacts' / 'run'
root.mkdir(parents=True, exist_ok=True)
processes, logs = {}, []
capture_process = None
dumped = set()
result = {'seconds': args.seconds, 'port': args.port, 'shutdown': {}}

def linux_setup():
    import resource
    resource.setrlimit(resource.RLIMIT_CORE, (-1, -1))
    # 이 테스트가 만든 자식만 디버거 접속을 허용합니다.
    ctypes.CDLL(None).prctl(0x59616d61, ctypes.c_ulong(-1), 0, 0, 0)

def launch(name, subdir, binary):
    source = args.build / subdir
    cwd = root / name
    cwd.mkdir(exist_ok=True)
    for item in source.iterdir():
        if item.suffix.lower() in ('.toml', '.txt', '.csv'):
            shutil.copy2(item, cwd / item.name)
    setting = cwd / ('serverSetting.toml' if name == 'server' else 'DummySetting.toml')
    text = setting.read_text(encoding='utf-8-sig')
    text = re.sub(r'(?im)^(port\s*=\s*)\d+', lambda m: m[1] + str(args.port), text)
    text = re.sub(r'(?im)^(ip\s*=\s*)"[^"]+"', lambda m: m[1] + '"127.0.0.1"', text)
    setting.write_text(text, encoding='utf-8')
    log = open(root / (name + '.log'), 'wb')
    logs.append(log)
    env = dict(os.environ, NETLIB_LATENCY_MS='100', TRACY_PORT='18086' if name == 'server' else '18087')
    command = [str((root / ('symbols-' + subdir) / (binary + ('.exe' if windows else ''))).resolve())]
    options = {}
    if windows:
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
        options.update(creationflags=subprocess.CREATE_NEW_CONSOLE, startupinfo=startup)
    else:
        command = ['stdbuf', '-oL', '-eL'] + command
        options.update(start_new_session=True, preexec_fn=linux_setup)
    p = subprocess.Popen(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT, env=env, **options)
    processes[name] = p
    return p

def windows_signal(pid):
    code = '''import ctypes, sys, time
k = ctypes.WinDLL('kernel32', use_last_error=True)
k.FreeConsole()
assert k.AttachConsole(int(sys.argv[1])), ctypes.get_last_error()
handler = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_uint)(lambda event: 1)
assert k.SetConsoleCtrlHandler(handler, True)
assert k.GenerateConsoleCtrlEvent(1, 0), ctypes.get_last_error()
time.sleep(0.3)
'''
    subprocess.run([sys.executable, '-c', code, str(pid)], check=True,
                   creationflags=subprocess.CREATE_NO_WINDOW, timeout=10)

def capture(name, p, reason='shutdown'):
    if not windows:
        command = ['gdb', '-q', '-batch', '-p', str(p.pid), '-ex', 'set pagination off',
                   '-ex', 'thread apply all bt full', '-ex', 'detach']
        with open(root / (name + '-stacks.txt'), 'w') as out:
            subprocess.run(command, stdout=out, stderr=subprocess.STDOUT, timeout=45)
    else:
        from ctypes import wintypes as w
        k = ctypes.WinDLL('kernel32', use_last_error=True)
        k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
        k.OpenProcess.restype = w.HANDLE
        k.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, w.LPVOID, w.DWORD, w.DWORD, w.HANDLE]
        k.CreateFileW.restype = w.HANDLE
        k.CloseHandle.argtypes = [w.HANDLE]
        dump = ctypes.WinDLL('Dbghelp').MiniDumpWriteDump
        dump.argtypes = [w.HANDLE, w.DWORD, w.HANDLE, w.DWORD, w.LPVOID, w.LPVOID, w.LPVOID]
        dump.restype = w.BOOL
        process = k.OpenProcess(0x0400 | 0x0010 | 0x0040 | 0x0800, False, p.pid)
        file = k.CreateFileW(str(root / (name + '-' + reason + '.dmp')), 0x40000000, 0, None, 2, 0, None)
        native = ctypes.WinDLL('ntdll')
        native.NtSuspendProcess.argtypes = [w.HANDLE]
        native.NtResumeProcess.argtypes = [w.HANDLE]
        suspended = False
        begin = time.time()
        try:
            # 덤프 작성 중에만 테스트 서버를 잠시 멈춰 스레드와 메모리 상태를 맞춥니다.
            if native.NtSuspendProcess(process) != 0:
                raise RuntimeError('Cannot suspend test process for dump')
            suspended = True
            if not dump(process, p.pid, file, 0x1000 | 0x40 | 0x4 | 0x2, None, None, None):
                raise ctypes.WinError(ctypes.get_last_error())
            from stacks import save_stacks
            save_stacks(process, p.pid, root / (name + '-' + reason + '-stacks.txt'), root / 'symbols-GameServer')
        finally:
            if suspended:
                native.NtResumeProcess(process)
            result.setdefault('dumps', []).append({'reason': reason, 'pid': p.pid, 'unix_start': begin, 'seconds': time.time()-begin})
            print('DUMP ' + reason + ' seconds=' + str(time.time()-begin), flush=True)
            k.CloseHandle(file)
            k.CloseHandle(process)

def stop(name):
    p = processes[name]
    start = time.monotonic()
    status = {'pid': p.pid}
    result['shutdown'][name] = status
    try:
        if p.poll() is not None:
            status['unexpected_exit'] = p.returncode
            return
        if windows:
            windows_signal(p.pid)
        else:
            p.send_signal(signal.SIGTERM)
        try:
            p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            status['timeout'] = True
            capture(name, p)
        status['exit_code'] = p.poll()
    except Exception as error:
        status['error'] = repr(error)
    finally:
        status['elapsed'] = round(time.monotonic() - start, 3)
        # 덤프/콜스택을 수집한 뒤 테스트에서 생성한 프로세스만 정리합니다.
        if p.poll() is None:
            p.kill()
            p.wait()
        print(json.dumps({name: status}), flush=True)

try:
    # 기존 프로세스를 종료하거나 같은 포트를 재사용하지 않습니다.
    with socket.socket() as probe:
        probe.bind(('127.0.0.1', args.port))
    if (root / 'started.json').exists():
        raise RuntimeError('The single authorized run has already been started')
    # 덤프와 정확히 일치하는 실행 파일 및 심볼을 먼저 보관합니다.
    for subdir in ('GameServer', 'Dummy'):
        symbols = root / ('symbols-' + subdir)
        symbols.mkdir(exist_ok=True)
        for item in (args.build / subdir).iterdir():
            if item.suffix.lower() in ('.exe', '.pdb', '.dll'):
                shutil.copy2(item, symbols / item.name)
    (root / 'started.json').write_text(json.dumps({'unix_start': time.time()}))
    run_started = time.monotonic()
    server = launch('server', 'GameServer', 'CMakeServer')
    deadline = time.monotonic() + 30
    while True:
        if server.poll() is not None:
            raise RuntimeError(f'server startup exit={server.returncode}')
        try:
            with socket.create_connection(('127.0.0.1', args.port), timeout=1):
                break
        except OSError:
            if time.monotonic() >= deadline:
                raise RuntimeError('server readiness timeout')
            time.sleep(1)
    dummy = launch('dummy', 'Dummy', 'Dummy')
    started = run_started
    while time.monotonic() - started < args.seconds:
        for name, p in processes.items():
            if p.poll() is not None:
                raise RuntimeError(f'{name} unexpected exit={p.returncode}')
        elapsed = time.monotonic() - started
        text = (root / 'server.log').read_text(errors='replace')
        target = None
        failures = {(m[0], int(m[2])): int(m[1]) for m in re.findall(r'\[SENDQ_EVENT\][^\n]*queue=(\S+)[^\n]*generation=(\d+) kind=fail ticket=(\d+)', text)}
        for line in text.splitlines():
            if '[SENDQ_PROBE]' not in line or 'reason=RealSend-confirm ' not in line: continue
            fields = dict(re.findall(r'(\w+)=(\S+)', line))
            if (fields['queue'], int(fields['head'])) not in failures: continue
            if int(fields['generation']) <= failures[(fields['queue'], int(fields['head']))]: continue
            if fields['active_before'] != '0' or fields['active_after'] != '0' or fields['epoch_before'] != fields['epoch_after']: continue
            if int(fields['occupied']) < 1: continue
            target = fields
            break
        if target:
            result['target'] = target
            # 목표 관측 직후 더미를 정지시켜 새로운 부하를 차단합니다.
            from stacks import suspend
            suspend(dummy.pid)
            result['target_observed'] = True
            result['load_seconds'] = time.monotonic() - started
            capture('server', server, 'send-stall')
            dummy.kill()
            dummy.wait()
            break
        print(f'LOAD elapsed={int(elapsed)} server={server.pid} dummy={dummy.pid}', flush=True)
        time.sleep(min(0.1, max(0, args.seconds - (time.monotonic() - started))))
    result.setdefault('load_seconds', time.monotonic() - started)
    result.setdefault('target_observed', False)
    if not result['target_observed']:
        from stacks import suspend
        suspend(dummy.pid)
        capture('server', server, 'time-limit')
        dummy.kill()
        dummy.wait()
    result['completed_load'] = True
except Exception as error:
    result['error'] = repr(error)
finally:
    for name in ('dummy', 'server'):
        if name in processes:
            stop(name)
    if capture_process:
        try:
            result['capture_exit'] = capture_process.wait(timeout=90)
        except subprocess.TimeoutExpired:
            result['capture_error'] = 'Capture did not finalize within 90 seconds'
            capture_process.kill()
            capture_process.wait()
    for log in logs:
        log.close()
    server_text = (root / 'server.log').read_text(errors='replace') if (root / 'server.log').exists() else ''
    dummy_text = (root / 'dummy.log').read_text(errors='replace') if (root / 'dummy.log').exists() else ''
    sessions = [int(n) for n in re.findall(r'Sessions\s*:\s*(\d+)', server_text)]
    result['peak_sessions'] = max(sessions, default=0)
    responses = re.findall(r'stage=dummy_response count=(\d+)', dummy_text)
    result['responses'] = int(responses[-1]) if responses else 0
    result['load_observed'] = result['peak_sessions'] >= 4500 and result['responses'] > 0
    result['passed'] = result.get('target_observed', False) and all(
        s.get('exit_code') == 0 and not s.get('timeout') for s in result['shutdown'].values()) and result['load_observed']
    (root / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result), flush=True)
sys.exit(0 if result['passed'] else 1)
