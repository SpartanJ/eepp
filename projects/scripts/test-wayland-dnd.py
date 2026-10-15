#!/usr/bin/env python3
"""Run the explicit Wayland drag checks in Weston inside an isolated Xvfb display.

Requires Weston with its X11 backend, Xvfb, xdotool and ImageMagick's import.
Build eepp-native-dnd-tests first. Normal/manual runs never inject input.
"""

import argparse
import io
import os
import re
from pathlib import Path
import selectors
import shutil
import signal
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def read_line(process, prefix, timeout=25):
    deadline = time.monotonic() + timeout
    # Unbuffered binary stdout avoids hidden read-ahead when selecting a pipe.
    with selectors.DefaultSelector() as selector:
        selector.register(process.stdout, selectors.EVENT_READ)
        pending = bytearray()
        while time.monotonic() < deadline:
            if not selector.select(max(0, deadline - time.monotonic())):
                break
            byte = process.stdout.read(1)
            if not byte:
                raise RuntimeError(f"Test process exited with {process.poll()}")
            if byte != b"\n":
                pending += byte
                continue
            line = pending.decode(errors="replace").strip()
            pending.clear()
            print(line, flush=True)
            if line.startswith(prefix):
                return line
    raise RuntimeError(f"Timed out waiting for {prefix}")


def pointer(*arguments):
    subprocess.run(["xdotool", *map(str, arguments)], check=True)


def color_point(color, compact=False):
    # PPM keeps this runner independent of Python image packages.
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        stream = io.BytesIO(
            subprocess.check_output(["import", "-window", "root", "-depth", "8", "ppm:-"])
        )
        assert stream.readline().strip() == b"P6"
        line = stream.readline()
        while line.startswith(b"#"):
            line = stream.readline()
        width, height = map(int, line.split())
        assert stream.readline().strip() == b"255"
        pixels = stream.read()
        wanted = bytes(color)
        # Window centers need room for the drag threshold; badges need only a small solid patch.
        radius = 2 if compact else 24
        candidates = []
        for y in range(60, height - 30, 6):
            for x in range(40, width - 40, 6):
                if all(
                    pixels[(yy * width + xx) * 3 : (yy * width + xx) * 3 + 3] == wanted
                    for xx, yy in (
                        (x, y),
                        (x - radius, y),
                        (x + radius, y),
                        (x, y + min(radius, 20)),
                    )
                ):
                    if compact:
                        return x, y
                    candidates.append((x, y))
        if candidates:
            # Prefer the middle of the visible window, keeping the preview away from output edges.
            center_x = (min(x for x, _ in candidates) + max(x for x, _ in candidates)) / 2
            center_y = (min(y for _, y in candidates) + max(y for _, y in candidates)) / 2
            return min(candidates, key=lambda p: (p[0] - center_x) ** 2 + (p[1] - center_y) ** 2)
        time.sleep(0.1)
    raise RuntimeError(f"No visible test surface with color {color}")


def run(binary, log_dir, scale):
    runtime = tempfile.mkdtemp(prefix="eepp-weston-")
    environment = dict(
        os.environ,
        XDG_RUNTIME_DIR=runtime,
        WAYLAND_DISPLAY="eepp-dnd-test",
        SDL_VIDEO_DRIVER="wayland",
        SDL_VIDEODRIVER="wayland",
        LIBGL_ALWAYS_SOFTWARE="1",
        SDL_VIDEO_WAYLAND_ALLOW_LIBDECOR="0",
    )
    children = []
    files = []

    def launch(arguments, name, protocol=False):
        log = (log_dir / (name + ".log")).open("wb")
        files.append(log)
        child_env = dict(environment)
        if protocol:
            child_env["WAYLAND_DEBUG"] = "1"
            # Only Weston uses X11. The applications must operate as pure Wayland clients.
            child_env.pop("DISPLAY", None)
            child_env.pop("XAUTHORITY", None)
        child = subprocess.Popen(
            arguments,
            cwd=ROOT,
            env=child_env,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=log,
            bufsize=0,
        )
        children.append(child)
        return child

    try:
        weston = launch(
            [
                "weston",
                "--backend=x11",
                "--renderer=pixman",
                f"--width={1240 // scale}",
                f"--height={760 // scale}",
                f"--scale={scale}",
                "--socket=eepp-dnd-test",
                "--no-config",
                "--idle-time=0",
                "--log=" + str(log_dir / "weston.log"),
            ],
            "weston-stderr",
        )
        deadline = time.monotonic() + 10
        while not Path(runtime, "eepp-dnd-test").exists():
            if weston.poll() is not None or time.monotonic() > deadline:
                raise RuntimeError("Weston could not start; inspect its log")
            time.sleep(0.1)
        tests = launch([str(binary), "--automated-wayland"], "source-protocol", True)
        for scenario in (
            "snapshot",
            "large",
            "rejected",
            "cancel",
            "escape",
            "destroy",
            "external",
            "instant",
            "stalled-cancel",
            "shutdown",
        ):
            ready = read_line(tests, "WAYLAND_DND_READY")
            assert ready.endswith(scenario), ready
            external = None
            if scenario == "external":
                external = launch(
                    [str(binary), "--automated-wayland-target"], "target-protocol", True
                )
                read_line(external, "WAYLAND_DND_EXTERNAL_READY")
            time.sleep(0.25)
            source = color_point((180, 30, 50))
            pointer("mousemove", *source, "click", 1)
            time.sleep(0.2)
            source = color_point((180, 30, 50))
            target = color_point((135, 45, 185) if external else (30, 80, 190))
            pointer("mousemove", *source, "mousedown", 1)
            time.sleep(0.1)
            pointer("mousemove", source[0] + 18, source[1] + 4)
            if scenario == "stalled-cancel":
                read_line(tests, "WAYLAND_DND_BEFORE_START")
                # Stop Weston before native setup, ensuring cancellation cannot use a cached reply.
                # Resume it before rendering: swap/frame callbacks may legitimately wait for it.
                weston.send_signal(signal.SIGSTOP)
                try:
                    tests.stdin.write(b"begin\n")
                    tests.stdin.flush()
                    read_line(tests, "WAYLAND_DND_STARTED", timeout=2)
                    tests.stdin.write(b"cleanup\n")
                    tests.stdin.flush()
                    read_line(tests, "WAYLAND_DND_CLEANUP_DONE", timeout=2)
                finally:
                    weston.send_signal(signal.SIGCONT)
            else:
                read_line(tests, "WAYLAND_DND_STARTED")
            if scenario == "escape":
                pointer("key", "Escape")
            elif scenario == "rejected":
                pointer("mousemove", 20, 600)
                color_point((195, 45, 55), compact=True)
            elif scenario not in ("cancel", "destroy", "instant", "stalled-cancel", "shutdown"):
                pointer("mousemove", *target)
                time.sleep(0.2)
                color_point((31, 150, 82), compact=True)
                if scenario == "snapshot":
                    pointer("mousemove", 20, 600)
                    color_point((195, 45, 55), compact=True)
                    pointer("mousemove", *target)
                    color_point((31, 150, 82), compact=True)
                if scenario in ("snapshot", "external"):
                    color_point((40, 220, 100))
                    subprocess.run(
                        ["import", "-window", "root", str(log_dir / "snapshot.png")], check=True
                    )
            pointer("mouseup", 1)
            result = read_line(tests, "WAYLAND_DND_RESULT")
            assert " PASS " in result, result
            if external:
                assert read_line(external, "WAYLAND_DND_EXTERNAL ").endswith("PASS")
                assert external.wait(timeout=5) == 0
            time.sleep(0.1)
            if scenario not in ("destroy", "shutdown"):
                tests.stdin.write(b"click\n")
                tests.stdin.flush()
                read_line(tests, "WAYLAND_DND_CLICK_READY")
                pointer("mousemove", *color_point((180, 30, 50)), "click", 1)
                assert read_line(tests, "WAYLAND_DND_CLICK ").endswith("PASS")
            if scenario != "shutdown":
                tests.stdin.write(b"next\n")
                tests.stdin.flush()
        assert tests.wait(timeout=5) == 0
        exiting = launch([str(binary), "--automated-wayland-exit"], "process-exit-protocol", True)
        read_line(exiting, "WAYLAND_DND_READY shutdown")
        time.sleep(0.25)
        source = color_point((180, 30, 50))
        pointer("mousemove", *source, "click", 1)
        time.sleep(0.2)
        source = color_point((180, 30, 50))
        pointer("mousemove", *source, "mousedown", 1)
        time.sleep(0.1)
        pointer("mousemove", source[0] + 18, source[1] + 4)
        read_line(exiting, "WAYLAND_DND_PROCESS_EXIT")
        assert exiting.wait(timeout=5) == 0
        pointer("mouseup", 1)
        print("Wayland process exit with a live backend passed")
        protocol = (log_dir / "source-protocol.log").read_text(errors="replace")
        # Older libwayland logs use '@' for object IDs; newer versions use '#'.
        created = len(re.findall(r"\.data_offer\(new id wl_data_offer[@#]", protocol))
        destroyed = len(re.findall(r"-> wl_data_offer[@#]\d+\.destroy\(\)", protocol))
        assert created > 0 and created == destroyed, (created, destroyed)
        print(f"Wayland offer proxies: {created} created, {destroyed} destroyed")
        print("Wayland drag checks passed; logs:", log_dir)
    except Exception:
        subprocess.run(["import", "-window", "root", str(log_dir / "failure.png")], check=False)
        raise
    finally:
        subprocess.run(
            ["xdotool", "mouseup", "1"],
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        for child in reversed(children):
            if child.poll() is None:
                child.terminate()
        for child in children:
            try:
                child.wait(timeout=3)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
        for file in files:
            file.close()
        shutil.rmtree(runtime)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inside-xvfb", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--binary", type=Path, default=ROOT / "bin/eepp-native-dnd-tests")
    parser.add_argument("--log-dir", type=Path)
    parser.add_argument("--scale", type=int, choices=(1, 2), default=1)
    args = parser.parse_args()
    if not args.inside_xvfb:
        return subprocess.call(
            [
                str(ROOT / "projects/scripts/xvfb-run-eepp"),
                sys.executable,
                str(Path(__file__).resolve()),
                "--inside-xvfb",
                *sys.argv[1:],
            ]
        )
    log_dir = (args.log_dir or Path(tempfile.mkdtemp(prefix="eepp-wayland-dnd-logs-"))).resolve()
    log_dir.mkdir(parents=True, exist_ok=True)
    print("Wayland test logs:", log_dir, flush=True)
    run(args.binary.resolve(), log_dir, args.scale)
    return 0


if __name__ == "__main__":
    sys.exit(main())
