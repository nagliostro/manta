"""Sends a payload (.elf) to the console's ELF loader (port 9021).

    python scripts/send-elf.py <PS5-IP> helper/pad_probe.elf
"""

import pathlib
import socket
import sys


def main() -> int:
    host, path = sys.argv[1], pathlib.Path(sys.argv[2])
    port = int(sys.argv[3]) if len(sys.argv) > 3 else 9021
    data = path.read_bytes()
    with socket.create_connection((host, port), timeout=15) as connection:
        connection.sendall(data)
    print(f"Sent {path.name} ({len(data)} bytes) to {host}:{port}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
