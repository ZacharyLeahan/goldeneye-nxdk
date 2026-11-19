"""Read a completed boot trace from xemu's local QMP monitor and verify its frame IDs."""
import argparse
import json
from pathlib import Path
import re
import socket
import struct
import tempfile

from frame_trace import summarize


def collect(map_file, port, destination):
    symbols = {}
    for line in map_file.read_text().splitlines():
        match = re.search(r"\s_([A-Za-z0-9_]+)\s+([0-9a-fA-F]+)\s+frame_trace\.c\.obj$", line)
        if match:
            symbols[match[1]] = int(match[2], 16)

    with socket.create_connection(("127.0.0.1", port), 10) as sock, sock.makefile("rwb") as monitor:
        def command(name, args=None):
            monitor.write((json.dumps({"execute": name, "arguments": args or {}}) + "\n").encode())
            monitor.flush()
            while True:
                reply = json.loads(monitor.readline())
                if "error" in reply:
                    raise ValueError(reply["error"])
                if "return" in reply:
                    return reply["return"]

        def word(name, displacement=0):
            address = symbols[name] + displacement
            output = command("human-monitor-command", {"command-line": f"x /wx 0x{address:x}"})
            return int(output.split(":", 1)[1].strip(), 16)

        if "QMP" not in json.loads(monitor.readline()):
            raise ValueError("Expected xemu QMP greeting")
        command("qmp_capabilities")
        if word("phase") != 4 or not word("records"):
            raise ValueError("Boot capture incomplete or marker absent")
        n = word("stopCount")
        hz = word("frequency") | word("frequency", 4) << 32
        if not 0 < n <= 73728 or not hz:
            raise ValueError("Invalid or overflowing boot trace")
        with tempfile.TemporaryDirectory() as temp:
            raw = Path(temp) / "records.bin"
            command("memsave", {"val": word("records"), "size": n*24, "filename": str(raw)})
            data = raw.read_bytes()
    if len(data) != n*24 or not any(kind == 17 for _,kind,_,_ in struct.iter_unpack("<QIIQ", data)):
        raise ValueError("Incomplete Xbox trace or missing hardware scanouts")
    destination.write_bytes(struct.pack("<IIIIQ", 0x31464547, n, 0, 24, hz) + data)
    summarize(destination, boot=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", dest="map_file", required=True, type=Path)
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    collect(args.map_file, args.port, args.output)
