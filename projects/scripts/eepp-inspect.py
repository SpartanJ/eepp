#!/usr/bin/env python3
"""Small standard-library client for the eepp inspector NDJSON protocol."""

import argparse
import json
import os
import queue
import select
import socket
import sys
import threading
import time


def emit(value, pretty=False):
    print(json.dumps(value, ensure_ascii=False, indent=2 if pretty else None,
                     separators=None if pretty else (",", ":")), flush=True)


class Client:
    def __init__(self, host, port, token):
        self.sock = socket.create_connection((host, port), timeout=10)
        self.sock.settimeout(None)
        self.input = b""
        self.next_id = 2
        reply = self.request({"id": 1, "method": "session.connect", "params": {
            "protocolVersion": 1, "token": token,
            "client": {"name": "eepp-inspect.py", "version": "1"}}})
        if "error" in reply:
            raise RuntimeError(reply["error"].get("message", str(reply["error"])))

    def close(self):
        self.sock.close()

    def send(self, request):
        request = dict(request)
        if "id" not in request:
            request["id"] = self.next_id
            self.next_id += 1
        self.sock.sendall((json.dumps(request, ensure_ascii=False,
                                      separators=(",", ":")) + "\n").encode("utf-8"))
        return request["id"]

    def receive(self):
        while b"\n" not in self.input:
            chunk = self.sock.recv(8192)
            if not chunk:
                raise EOFError("inspector disconnected")
            self.input += chunk
            if len(self.input) > 4 * 1024 * 1024:
                raise ValueError("response exceeds 4 MiB")
        line, self.input = self.input.split(b"\n", 1)
        return json.loads(line.decode("utf-8").rstrip("\r"))

    def readable(self, timeout=None):
        return b"\n" in self.input or bool(select.select([self.sock], [], [], timeout)[0])

    def request(self, request, on_event=None):
        request_id = self.send(request)
        while True:
            message = self.receive()
            if message.get("id") == request_id:
                return message
            if on_event is not None:
                on_event(message)


def target(value, scene=None):
    return {"handle": value} if value.startswith("w:") else {
        **({"scene": scene} if scene else {}), "selector": value}


def scene_param(args):
    return {"scene": args.scene} if getattr(args, "scene", None) else {}


def command(args):
    name = args.command
    params = scene_param(args)
    if name == "contexts":
        return "ui.contexts", {}
    if name == "query":
        params.update(selector=args.selector, offset=args.offset, limit=args.limit)
        if args.properties:
            params["properties"] = args.properties
        if args.max_string_length is not None:
            params["maxStringLength"] = args.max_string_length
        return "ui.query", params
    if name == "tree":
        params.update(depth=args.depth, maxNodes=args.max_nodes,
                      includeText=args.include_text)
        if args.root:
            params = {key: value for key, value in params.items() if key != "scene"}
            params["root"] = args.root
        return "ui.tree", params
    if name == "inspect":
        params.update(target(args.target, args.scene))
        if args.properties:
            params["properties"] = args.properties
        if args.max_string_length is not None:
            params["maxStringLength"] = args.max_string_length
        return "ui.inspect", params
    if name == "keybindings":
        params = target(args.target, args.scene) if args.target else params
        params.update(offset=args.offset, limit=args.limit)
        return "ui.keybindings", params
    if name == "focus":
        return "ui.focus", params
    if name == "screenshot":
        if args.window:
            params["window"] = args.window
        if args.rect:
            params["rect"] = args.rect
        params["format"] = args.format
        return "ui.screenshot", params
    if name == "click":
        params = target(args.target, args.scene)
        params["button"] = args.button
        return "input.click", params
    if name == "key":
        params.update(key=args.key, action=args.action, modifiers=args.modifiers)
        return "input.key", params
    if name == "type":
        params["text"] = args.text
        return "input.text", params
    if name == "watch":
        params.update(target(args.target, args.scene))
        params.update(properties=args.properties, initial=not args.no_initial)
        return "watch.subscribe", params
    if name == "batch":
        with (sys.stdin if args.file == "-" else open(args.file, encoding="utf-8")) as stream:
            return "session.batch", json.load(stream)
    if name == "raw":
        return None, json.loads(args.request)
    raise ValueError(name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default=os.getenv("EEPP_INSPECTOR_HOST", "127.0.0.1"))
    parser.add_argument("--port", type=int, default=int(os.getenv("EEPP_INSPECTOR_PORT", "0")))
    parser.add_argument("--token", default=os.getenv("EEPP_INSPECTOR_TOKEN"))
    parser.add_argument("--pretty", action="store_true")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("contexts")
    query = sub.add_parser("query")
    query.add_argument("selector")
    query.add_argument("--scene")
    query.add_argument("--offset", type=int, default=0)
    query.add_argument("--limit", type=int, default=50)
    query.add_argument("--properties", nargs="+")
    query.add_argument("--max-string-length", type=int)
    tree = sub.add_parser("tree")
    tree.add_argument("--scene")
    tree.add_argument("--root")
    tree.add_argument("--depth", type=int, default=3)
    tree.add_argument("--max-nodes", type=int, default=200)
    tree.add_argument("--include-text", action="store_true")
    inspect = sub.add_parser("inspect")
    inspect.add_argument("target", help="widget handle or selector")
    inspect.add_argument("properties", nargs="*")
    inspect.add_argument("--scene")
    inspect.add_argument("--max-string-length", type=int)
    keybindings = sub.add_parser("keybindings", help="list bindings on a scene or widget")
    keybindings.add_argument("target", nargs="?", help="widget handle or selector; omit for scene")
    keybindings.add_argument("--scene")
    keybindings.add_argument("--offset", type=int, default=0)
    keybindings.add_argument("--limit", type=int, default=50)
    focus = sub.add_parser("focus")
    focus.add_argument("--scene")
    screenshot = sub.add_parser("screenshot", help="save a window or scene image to a temporary path")
    screenshot_context = screenshot.add_mutually_exclusive_group()
    screenshot_context.add_argument("--scene", help="capture the scene's visible rectangle")
    screenshot_context.add_argument("--window", help="capture this native window")
    screenshot.add_argument("--rect", nargs=4, type=int, metavar=("X", "Y", "WIDTH", "HEIGHT"),
                            help="capture a rectangle in native-window pixel coordinates")
    screenshot.add_argument("--format", choices=("png", "jpg", "bmp", "tga", "qoi", "webp"),
                            default="png")
    click = sub.add_parser("click")
    click.add_argument("target")
    click.add_argument("--scene")
    click.add_argument("--button", choices=("left", "middle", "right"), default="left")
    key = sub.add_parser("key")
    key.add_argument("key")
    key.add_argument("--scene")
    key.add_argument("--action", choices=("press", "down", "up"), default="press")
    key.add_argument("--modifiers", nargs="*", default=[])
    typ = sub.add_parser("type", help="inject text; optional --target clicks first")
    typ.add_argument("text")
    typ.add_argument("--scene")
    typ.add_argument("--target")
    watch = sub.add_parser("watch")
    watch.add_argument("target")
    watch.add_argument("properties", nargs="+")
    watch.add_argument("--scene")
    watch.add_argument("--no-initial", action="store_true")
    watch.add_argument("--timeout", type=float)
    watch.add_argument("--count", type=int)
    batch = sub.add_parser("batch")
    batch.add_argument("file", help="JSON params file, or - for stdin")
    sub.add_parser("session", help="authenticated persistent NDJSON stdin/stdout")
    raw = sub.add_parser("raw")
    raw.add_argument("request", help="JSON request object; id is assigned if omitted")
    args = parser.parse_args()
    if not args.port or not args.token:
        parser.error("--port and --token (or their environment variables) are required")
    client = Client(args.host, args.port, args.token)
    try:
        if args.command == "session":
            requests = queue.Queue()
            def read_stdin():
                for line in sys.stdin:
                    requests.put(line)
                requests.put(None)
            threading.Thread(target=read_stdin, daemon=True).start()
            stdin_open = True
            outstanding = set()
            while stdin_open or outstanding:
                try:
                    line = requests.get_nowait()
                    if line is None:
                        stdin_open = False
                    elif line.strip():
                        outstanding.add(client.send(json.loads(line)))
                except queue.Empty:
                    pass
                if client.readable(0.05):
                    message = client.receive()
                    emit(message)
                    outstanding.discard(message.get("id"))
            return
        if args.command == "type" and args.target:
            reply = client.request({"method": "input.click",
                                    "params": target(args.target, args.scene)})
            if "error" in reply:
                emit(reply, args.pretty)
                return 1
            if not args.scene:
                args.scene = reply["result"].get("scene")
        method, params = command(args)
        request = params if method is None else {"method": method, "params": params}
        reply = client.request(request)
        emit(reply, args.pretty)
        if "error" in reply:
            return 1
        if args.command == "watch":
            deadline = time.monotonic() + args.timeout if args.timeout is not None else None
            seen = 0
            while args.count is None or seen < args.count:
                remaining = None if deadline is None else max(0, deadline - time.monotonic())
                if remaining == 0 or not client.readable(remaining):
                    break
                event = client.receive()
                emit(event, args.pretty)
                if event.get("method") == "watch.changed":
                    seen += 1
    finally:
        client.close()
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, EOFError, RuntimeError, json.JSONDecodeError) as exc:
        print(f"eepp-inspect: {exc}", file=sys.stderr)
        sys.exit(2)
