#!/usr/bin/env python3
"""Exercise a real native drag into a browser file input. Run under an isolated X server.
Requires the native-DND example, xdotool, and Chromium or Firefox; no Python packages.
"""
import argparse
import json
import pathlib
import shutil
import subprocess
import tempfile
import time


def xdo(*args):
    return subprocess.check_output(["xdotool", *map(str, args)], text=True).strip()


def find_window(title, timeout=15):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = subprocess.run(["xdotool", "search", "--name", title], capture_output=True, text=True)
        if result.returncode == 0:
            return result.stdout.splitlines()[0]
        time.sleep(0.1)
    raise RuntimeError(f"Window did not appear: {title}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default="bin/eepp-native-drag-and-drop")
    parser.add_argument("--browser", required=True, help="Path to the browser binary")
    parser.add_argument("--firefox", action="store_true")
    parser.add_argument("--single", action="store_true")
    args = parser.parse_args()
    if not shutil.which("xdotool"):
        parser.error("xdotool is required")
    source_binary = str(pathlib.Path(args.source).resolve())
    with tempfile.TemporaryDirectory(prefix="eepp-dnd-browser-") as directory:
        root = pathlib.Path(directory)
        names = ["drag #é-one.txt"] if args.single else ["drag #é-one.txt", "drag two.txt"]
        contents = ["first file\n"] if args.single else ["first file\n", "second file\n"]
        files = []
        for name, content in zip(names, contents):
            file = root / name
            file.write_text(content, encoding="utf-8")
            files.append(str(file))
        fixture = root / "upload.html"
        fixture.write_text("""<!doctype html><meta charset="utf-8"><title>eepp DND acceptance ready</title>
<style>body{margin:0;font:20px sans-serif}input{display:block;width:95%;height:400px;border:3px dashed blue}</style>
<p>Drop files into this native file input</p><input type="file" multiple><pre id="result">Waiting</pre>
<script>document.querySelector('input').onchange=async e=>{
const names=Array.from(e.target.files,f=>f.name);
const contents=await Promise.all(Array.from(e.target.files,f=>f.text()));
const passed=JSON.stringify(names)===JSON.stringify(EXPECTED_NAMES) && JSON.stringify(contents)===JSON.stringify(EXPECTED_CONTENTS);
document.title=passed?'eepp DND acceptance PASS':'eepp DND acceptance FAIL '+JSON.stringify({names,contents});
document.querySelector('#result').textContent=JSON.stringify({names,contents});};</script>
""".replace("EXPECTED_NAMES", json.dumps(names)).replace(
            "EXPECTED_CONTENTS", json.dumps(contents)), encoding="utf-8")
        profile = root / "profile"
        profile.mkdir()
        if args.firefox:
            (profile / "user.js").write_text('user_pref("layout.css.devPixelsPerPx", "1.0");\n')
            browser_command = [args.browser, "--no-remote", "--profile", str(profile), "--width", "600", "--height", "700", fixture.as_uri()]
        else:
            browser_command = [args.browser, "--no-sandbox", "--no-first-run", "--force-device-scale-factor=1", f"--user-data-dir={profile}", "--window-size=600,700", f"--app={fixture.as_uri()}"]
        with (root / "browser.log").open("w") as browser_log, (root / "source.log").open("w") as source_log:
            browser = subprocess.Popen(browser_command, stdout=browser_log, stderr=subprocess.STDOUT)
            source = subprocess.Popen([source_binary, *files], stdout=source_log, stderr=subprocess.STDOUT)
            destination = None
            try:
                destination = find_window("eepp DND acceptance ready")
                origin = find_window("eepp - Native file drag")
                xdo("windowmove", origin, 10, 650)
                xdo("windowmove", destination, 660, 0)
                xdo("windowraise", origin)
                xdo("windowfocus", origin)
                time.sleep(0.3)
                xdo("mousemove", "--window", origin, 100, 100, "mousedown", 1, "sleep", 0.3,
                    "mousemove", "--window", origin, 125, 100, "sleep", 0.5,
                    "mousemove", "--window", destination, 300, 300, "sleep", 0.7, "mouseup", 1)
                find_window("eepp DND acceptance PASS", timeout=5)
                print(f"PASS: {len(files)} files, Unicode filename and exact file contents")
            except Exception:
                source_log.flush()
                if destination is not None:
                    print(xdo("getwindowname", destination))
                print((root / "source.log").read_text())
                raise
            finally:
                xdo("mouseup", 1)
                for process in (source, browser):
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()


if __name__ == "__main__":
    main()
