"""Minimal raw TCP differential: logs bytes sent and bytes actually received.
Usage: python tests/echo_transport_probe.py --output build/echo-diagnostics/raw.jsonl
"""
import argparse
import json
from pathlib import Path
import socket
import struct
import threading


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    mismatches = 0
    with args.output.open('w', encoding='utf-8') as log:
        for value in (0, 1199, 1200, 1201, 1400):
            with socket.socket() as listener:
                listener.bind(('127.0.0.1', 0))
                listener.listen()
                listener.settimeout(3)
                packet = struct.pack('<HQ', 8, value)
                assert packet == (8).to_bytes(2, 'little') + value.to_bytes(8, 'little')
                def send():
                    with socket.create_connection(listener.getsockname(), timeout=3) as sock:
                        sock.sendall(packet)
                        sock.shutdown(socket.SHUT_WR)
                worker = threading.Thread(target=send)
                worker.start()
                received = bytearray()
                outcome = 'eof'
                try:
                    with listener.accept()[0] as connection:
                        connection.settimeout(0.3)
                        try:
                            while chunk := connection.recv(1024):
                                received.extend(chunk)
                        except (TimeoutError, ConnectionResetError) as error:
                            outcome = repr(error)
                finally:
                    worker.join(timeout=4)
                same = received == packet
                mismatches += not same
                event = dict(value=value, expected=packet.hex(), received=received.hex(), equal=same, outcome=outcome)
                log.write(json.dumps(event) + '\n')
                log.flush()
                print(json.dumps(event), flush=True)
    return int(mismatches != 0)


if __name__ == '__main__':
    raise SystemExit(main())
