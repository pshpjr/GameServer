"""Validate the LAN echo probe independently, then diagnose NetLib over TCP.

python tests/echo_probe.py --self-test
python tests/echo_probe.py --server out/build/verified-Debug/NetLibTest_Echo/NetLibTest_Echo.exe --clients 16 --rounds 5
No custom classes, third-party packages, or platform-specific networking APIs.
"""
import argparse
import concurrent.futures
import json
from pathlib import Path
import socket
import struct
import subprocess
import threading
import time
from unittest.mock import MagicMock, patch


CLIENT_OFFSET = 0

def record(log, lock, **event):
    with lock:
        log.write(json.dumps({'time_ns': time.monotonic_ns(), **event}) + '\n')
        log.flush()


def receive_exact(sock, size):
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise EOFError(f'expected={size} received={len(data)}')
        data.extend(chunk)
    return bytes(data)


def client(index, address, count, barrier, log, lock, legacy=False):
    index += CLIENT_OFFSET
    received = 0
    sent = 0
    try:
        # Explicit wire-format oracle independent of the reference server.
        assert struct.pack('<HQ', 8, 0x0807060504030201) == bytes(range(8, 9)) + b'\x00' + bytes(range(1, 9))
        values = list(range(index * count, (index + 1) * count)) if legacy else [((index + 1) << 48) | n for n in range(count)]
        packets = b''.join(struct.pack('<HQ', 8, value) for value in values)
        with socket.create_connection(address, timeout=5) as sock:
            sock.settimeout(5)
            record(log, lock, event='connected', client=index, local=sock.getsockname(), peer=sock.getpeername())
            if barrier:
                barrier.wait(timeout=6)
            sock.sendall(packets)
            sent = len(packets)
            record(log, lock, event='sent', client=index, bytes=sent, packets=count)
            if legacy:
                response = receive_exact(sock, len(packets))
                assert response == packets, 'Raw response differs from request'
                record(log, lock, event='passed', client=index, sent=sent, received=len(response))
                return True
            for expected in values:
                header = receive_exact(sock, 2)
                received += 2
                length = struct.unpack('<H', header)[0]
                if length != 8:
                    raise AssertionError(f'bad_header length={length} bytes={header.hex()}')
                payload = receive_exact(sock, length)
                received += length
                value = struct.unpack('<Q', payload)[0]
                if value != expected:
                    raise AssertionError(f'bad_payload expected={expected:#x} actual={value:#x}')
            record(log, lock, event='passed', client=index, sent=sent, received=received)
        return True
    except Exception as error:
        record(log, lock, event='failed', client=index, sent=sent, received=received, error=repr(error))
        return False


def run_clients(address, clients, packets, synchronized, log, lock, legacy=False):
    barrier = threading.Barrier(clients) if synchronized else None
    with concurrent.futures.ThreadPoolExecutor(max_workers=clients) as pool:
        futures = [pool.submit(client, n, address, packets, barrier, log, lock, legacy) for n in range(clients)]
        return sum(f.result() for f in futures)


def reference_connection(sock, packets, fault):
    with sock:
        sock.settimeout(8)
        response = bytearray()
        for _ in range(packets):
            header = receive_exact(sock, 2)
            assert int.from_bytes(header, 'little') == 8, f'Reference received invalid header: {header.hex()}'
            body = receive_exact(sock, 8)
            response.extend((8).to_bytes(2, 'little'))
            response.extend(int.from_bytes(body, 'little').to_bytes(8, 'little'))
        if fault == 'corrupt':
            response[-1] ^= 1
        if fault == 'truncate':
            response = response[:-3]
        sock.sendall(response)
        sock.shutdown(socket.SHUT_WR)
        while sock.recv(1024):
            pass


def reference_trial(clients, packets, log, lock, fault=None, legacy=False):
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        listener.listen(clients)
        listener.settimeout(8)
        address = listener.getsockname()
        def serve():
            with concurrent.futures.ThreadPoolExecutor(max_workers=clients) as pool:
                futures = [pool.submit(reference_connection, listener.accept()[0], packets, fault) for _ in range(clients)]
                for future in futures:
                    future.result()
        with concurrent.futures.ThreadPoolExecutor(max_workers=1) as server_pool:
            server = server_pool.submit(serve)
            passed = run_clients(address, clients, packets, True, log, lock, legacy)
            server.result()
        return passed


def validate_client_in_memory(log, lock):
    # No TCP or reference server: check framing, recv fragmentation, and failures.
    for legacy in (False, True):
        index = CLIENT_OFFSET
        values = list(range(index * 2, index * 2 + 2)) if legacy else [((index + 1) << 48) | n for n in range(2)]
        expected = b''.join((8).to_bytes(2, 'little') + value.to_bytes(8, 'little') for value in values)
        for fault in (None, 'corrupt', 'truncate', 'header'):
            response = bytearray(expected)
            if fault == 'corrupt':
                response[-1] ^= 1
            elif fault == 'truncate':
                del response[-3:]
            elif fault == 'header':
                response[0] = 9
            position = 0
            def recv(size):
                nonlocal position
                count = min(size, 1 + position % 3)
                part = bytes(response[position:position + count])
                position += len(part)
                return part
            sock = MagicMock()
            sock.__enter__.return_value = sock
            sock.getsockname.return_value = ('fixture', 1)
            sock.getpeername.return_value = ('fixture', 2)
            sock.recv.side_effect = recv
            with patch('socket.create_connection', return_value=sock):
                passed = client(0, ('fixture', 2), 2, None, log, lock, legacy)
            sock.sendall.assert_called_once_with(expected)
            assert passed == (fault is None), (legacy, fault, passed)
    print('IN-MEMORY: framing, partial reads, corruption, truncation and invalid header passed', flush=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--client-unit', action='store_true')
    parser.add_argument('--server', type=Path)
    parser.add_argument('--client-offset', type=int, default=0)
    parser.add_argument('--legacy', action='store_true')
    parser.add_argument('--port', type=int, default=6001)
    parser.add_argument('--trace', action='store_true')
    parser.add_argument('--clients', type=int, default=16)
    parser.add_argument('--packets', type=int, default=200)
    parser.add_argument('--rounds', type=int, default=5)
    parser.add_argument('--unsynchronized', action='store_true')
    parser.add_argument('--output', type=Path, default=Path('build/echo-diagnostics'))
    args = parser.parse_args()
    global CLIENT_OFFSET
    CLIENT_OFFSET = args.client_offset
    args.output.mkdir(parents=True, exist_ok=True)
    lock = threading.Lock()
    failures = 0
    with (args.output / 'clients.jsonl').open('w', encoding='utf-8') as log:
        if args.client_unit:
            validate_client_in_memory(log, lock)
            return 0
        if args.self_test:
            validate_client_in_memory(log, lock)
            for n in range(args.rounds):
                passed = reference_trial(args.clients, args.packets, log, lock, legacy=args.legacy)
                assert passed == args.clients, (n, passed)
                print(f'REFERENCE round={n} passed={passed}/{args.clients}', flush=True)
            for fault in ('corrupt', 'truncate'):
                assert reference_trial(1, 2, log, lock, fault) == 0, f'Client missed {fault}'
                print(f'NEGATIVE CONTROL detected={fault}', flush=True)
            return 0
        if args.server is None:
            parser.error('--server or --self-test is required')
        exe = args.server.resolve()
        for n in range(args.rounds):
            with socket.socket() as probe:
                probe.bind(('0.0.0.0', args.port))
            with (args.output / f'server-{n}.log').open('w') as server_log:
                # CREATE_NO_WINDOW is only a process-launch convenience on Windows.
                process = subprocess.Popen([str(exe), str(args.port)] + (['--trace'] if args.trace else []), cwd=exe.parent, stdout=server_log, stderr=server_log,
                                           creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
                try:
                    time.sleep(0.7)
                    assert process.poll() is None, f'server_exit={process.returncode}'
                    record(log, lock, event='round', round=n, server_pid=process.pid)
                    passed = run_clients(('127.0.0.1', args.port), args.clients, args.packets, not args.unsynchronized, log, lock, args.legacy)
                    failures += args.clients - passed
                    print(f'NETLIB round={n} passed={passed}/{args.clients} process_exit={process.poll()}', flush=True)
                    time.sleep(0.1)
                finally:
                    if process.poll() is None:
                        process.terminate()
                    process.wait(timeout=10)
    return int(failures != 0)


if __name__ == '__main__':
    raise SystemExit(main())
