"""Transport and CLI smoke tests using a small fake NDJSON inspector."""

import json
import pathlib
import socket
import subprocess
import sys
import threading
import unittest


CLIENT = pathlib.Path(__file__).resolve().parents[1] / "eepp-inspect.py"


class FakeServer:
    def __init__(self, handler):
        self.listener = socket.socket()
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen(1)
        self.port = self.listener.getsockname()[1]
        self.handler = handler
        self.error = None
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.thread.start()

    def run(self):
        try:
            with self.listener:
                conn, _ = self.listener.accept()
                with conn, conn.makefile("r", encoding="utf-8") as lines:
                    connect = json.loads(lines.readline())
                    assert connect["method"] == "session.connect"
                    assert connect["params"]["token"] == "test-token"
                    conn.sendall(b'{"id":1,"result":{"protocolVersion":1}}\n')
                    self.handler(conn, lines)
        except Exception as exc:
            self.error = exc

    def join(self):
        self.thread.join(timeout=5)
        if self.error:
            raise self.error
        assert not self.thread.is_alive(), "fake server did not finish"


def invoke(server, *args, input_text=None):
    return subprocess.run(
        [sys.executable, str(CLIENT), "--port", str(server.port), "--token", "test-token", *args],
        input=input_text, text=True, capture_output=True, timeout=5,
    )


class InspectorCliTests(unittest.TestCase):
    def test_contexts_emits_compact_json(self):
        def handler(conn, lines):
            request = json.loads(lines.readline())
            self.assertEqual(request["method"], "ui.contexts")
            conn.sendall((json.dumps({"id": request["id"], "result": {"scenes": []}}) + "\n").encode())

        server = FakeServer(handler)
        result = invoke(server, "contexts")
        server.join()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["result"]["scenes"], [])
        self.assertEqual(result.stdout.count("\n"), 1)

    def test_batch_reads_stdin_and_preserves_commands(self):
        def handler(conn, lines):
            request = json.loads(lines.readline())
            self.assertEqual(request["method"], "session.batch")
            self.assertFalse(request["params"]["commands"][0]["return"])
            conn.sendall((json.dumps({"id": request["id"], "result": {"results": {}}}) + "\n").encode())

        server = FakeServer(handler)
        definition = '{"commands":[{"method":"ui.query","return":false,"params":{"selector":"*"}}]}'
        result = invoke(server, "batch", "-", input_text=definition)
        server.join()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["result"]["results"], {})

    def test_screenshot_sends_window_rect_and_format(self):
        def handler(conn, lines):
            request = json.loads(lines.readline())
            self.assertEqual(request["method"], "ui.screenshot")
            self.assertEqual(request["params"], {
                "window": "win:2", "rect": [5, 6, 30, 20], "format": "qoi"})
            conn.sendall((json.dumps({"id": request["id"], "result": {
                "path": "/tmp/example.qoi", "format": "qoi"}}) + "\n").encode())

        server = FakeServer(handler)
        result = invoke(server, "screenshot", "--window", "win:2", "--rect",
                        "5", "6", "30", "20", "--format", "qoi")
        server.join()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["result"]["path"], "/tmp/example.qoi")

    def test_session_streams_events_between_pipelined_responses(self):
        def handler(conn, lines):
            first = json.loads(lines.readline())
            second = json.loads(lines.readline())
            self.assertEqual(first["method"], "ui.contexts")
            self.assertEqual(second["method"], "ui.focus")
            messages = [
                {"method": "watch.changed", "params": {"sequence": 1}},
                {"id": second["id"], "result": {"widget": None}},
                {"id": first["id"], "result": {"scenes": []}},
            ]
            conn.sendall("".join(json.dumps(message) + "\n" for message in messages).encode())

        server = FakeServer(handler)
        requests = '{"method":"ui.contexts"}\n{"method":"ui.focus"}\n'
        result = invoke(server, "session", input_text=requests)
        server.join()
        self.assertEqual(result.returncode, 0, result.stderr)
        messages = [json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(messages[0]["method"], "watch.changed")
        self.assertEqual({message.get("id") for message in messages[1:]}, {2, 3})


if __name__ == "__main__":
    unittest.main()
