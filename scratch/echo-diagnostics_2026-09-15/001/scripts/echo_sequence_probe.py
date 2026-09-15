"""One TCP connection per case; wait for each exact echo before sending the next.
Run from the repository root: python tests/echo_sequence_probe.py
"""
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import time

from echo_probe import receive_exact


def main():
    exe = Path('out/build/verified-Debug/NetLibTest_Echo/NetLibTest_Echo.exe').resolve()
    output = Path('build/echo-diagnostics/sequence').resolve()
    output.mkdir(parents=True, exist_ok=True)
    results = []
    for count in (1201, 100, 50):
        with socket.socket() as probe:
            probe.bind(('0.0.0.0', 6001))
        server_path = output / f'server-{count}.log'
        client_path = output / f'client-{count}.jsonl'
        result = dict(packets=count, first_value=0, last_value=count - 1, echoed=0, passed=False)
        with server_path.open('w') as server_log, client_path.open('w', encoding='utf-8') as client_log:
            env = dict(os.environ, NETLIB_TRACE_RECV='1')
            process = subprocess.Popen([str(exe), '6001', '--trace'], cwd=exe.parent,
                                       env=env, stdout=server_log, stderr=server_log,
                                       creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
            try:
                time.sleep(0.7)
                assert process.poll() is None, f'server_exit={process.returncode}'
                with socket.create_connection(('127.0.0.1', 6001), timeout=5) as client:
                    client.settimeout(5)
                    client_log.write(json.dumps(dict(event='connected', local=client.getsockname(), peer=client.getpeername())) + '\n')
                    for value in range(count):
                        packet = struct.pack('<HQ', 8, value)
                        assert packet == (8).to_bytes(2, 'little') + value.to_bytes(8, 'little')
                        client_log.write(json.dumps(dict(event='send', ordinal=value + 1, value=value, hex=packet.hex())) + '\n')
                        client_log.flush()
                        client.sendall(packet)
                        response = receive_exact(client, len(packet))
                        client_log.write(json.dumps(dict(event='recv', ordinal=value + 1, value=value, hex=response.hex())) + '\n')
                        client_log.flush()
                        assert response == packet, f'value={value} expected={packet.hex()} actual={response.hex()}'
                        result['echoed'] += 1
                time.sleep(0.2)
                assert process.poll() is None, f'server_exit={process.returncode}'
                result['passed'] = True
            except Exception as error:
                result['error'] = repr(error)
                client_log.write(json.dumps(dict(event='failed', **result)) + '\n')
            finally:
                if process.poll() is None:
                    process.terminate()
                process.wait(timeout=10)
        text = server_path.read_text(errors='replace')
        result['server_http2'] = 'HTTP/2.0' in text
        result['invalid_length'] = 'event=invalid_length' in text
        result['server_received_packets'] = text.count('event=recv ')
        result['server_connections'] = text.count('event=connect ')
        result['server_saw_1200'] = 'event=recv session=1 index=0 value=1200 error=0' in text
        results.append(result)
        print(json.dumps(result), flush=True)
    (output / 'summary.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    return int(any(not result['passed'] for result in results))


if __name__ == '__main__':
    raise SystemExit(main())
