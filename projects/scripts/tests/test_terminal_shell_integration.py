"""Check directory encoding and startup hooks using installed shells and isolated homes."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from urllib.parse import quote

ROOT = Path(__file__).resolve().parents[3]
INTEGRATION = ROOT / "bin/assets/shell-integration"
SHELLS = sorted({str(Path(p).resolve()) for p in
                 (shutil.which("bash"), shutil.which("zsh"), "/bin/bash") if p and Path(p).exists()})
POWERSHELL = os.environ.get("ETERM_TEST_PWSH") or shutil.which("pwsh") or shutil.which("powershell")


class ShellIntegration(unittest.TestCase):
    def test_directory_encoding_and_exit_status(self):
        with tempfile.TemporaryDirectory(prefix="eterm shell ") as temporary:
            directory = Path(temporary).resolve() / "space #é'[]"
            directory.mkdir()
            expected = "\x1b]7;file://localhost" + quote(str(directory), safe="/._~-") + "\x07"
            for shell in SHELLS:
                with self.subTest(shell=shell):
                    command = 'source "$1"; __eterm_report_working_directory; false; __eterm_report_working_directory >/dev/null'
                    result = subprocess.run([shell, "-c", command, "check",
                                             str(INTEGRATION / "directory-reporting.sh")],
                                            cwd=directory, capture_output=True, text=True, timeout=5)
                    self.assertEqual(result.stdout, expected)
                    self.assertEqual(result.stderr, "")
                    self.assertEqual(result.returncode, 1)

    def test_startup_files_and_existing_hooks(self):
        for shell in SHELLS:
            with self.subTest(shell=shell), tempfile.TemporaryDirectory(prefix="eterm startup ") as temporary:
                home = Path(temporary).resolve()
                child = home / "child #é"
                child.mkdir()
                env = dict(os.environ, HOME=str(home), ETERM_INTEGRATION_DIR=str(INTEGRATION),
                           ETERM_SHELL_INTEGRATION="1", TERM="xterm-256color")
                if Path(shell).name.startswith("bash"):
                    (home / ".bashrc").write_text('PROMPT_COMMAND=\'printf "user-hook\\n"\'\n')
                    env.update(ETERM_BASH_NOPROFILE="0", ETERM_BASH_NORC="0", ETERM_BASH_RCFILE="")
                    args = ["--noprofile", "--rcfile", str(INTEGRATION / "bash.bash"), "-i"]
                else:
                    dotdir = home / "custom"
                    dotdir.mkdir()
                    (home / ".zshenv").write_text('export ZDOTDIR="$HOME/custom"\n')
                    (dotdir / ".zshrc").write_text('precmd() { printf "user-hook\\n"; }\n'
                                                  'print -r -- "$HISTFILE" > "$HOME/history-file"\n')
                    env.update(ZDOTDIR=str(INTEGRATION / "zsh"), ETERM_ZDOTDIR_SET="0")
                    args = ["-i"]
                result = subprocess.run([shell, *args], input=f"cd '{child}'\nexit\n",
                                        cwd=home, env=env, capture_output=True, text=True, timeout=5)
                for directory in (home, child):
                    self.assertIn("\x1b]7;file://localhost" + quote(str(directory), safe="/._~-") + "\x07",
                                  result.stdout)
                self.assertGreaterEqual(result.stdout.count("user-hook"), 2)
                if not Path(shell).name.startswith("bash"):
                    history = (home / "history-file").read_text().strip()
                    if history:
                        self.assertTrue(Path(history).is_relative_to(home), history)

    @unittest.skipUnless(POWERSHELL, "PowerShell is not installed")
    def test_powershell_prompt_encoding_status_and_providers(self):
        with tempfile.TemporaryDirectory(prefix="eterm powershell ") as temporary:
            directory = Path(temporary).resolve() / "space #é'[]%"
            directory.mkdir()
            # Environment paths avoid embedding test filesystem names in PowerShell source.
            env = dict(os.environ, ETERM_TEST_INTEGRATION=str(INTEGRATION / "powershell.ps1"),
                       ETERM_TEST_DIRECTORY=str(directory), ETERM_SHELL_INTEGRATION="1")
            script = '''
function global:prompt { "user-hook:$?/$global:LASTEXITCODE" }
. $env:ETERM_TEST_INTEGRATION
. $env:ETERM_TEST_INTEGRATION
Set-Location -LiteralPath $env:ETERM_TEST_DIRECTORY
$global:LASTEXITCODE = 17
Write-Error failure -ErrorAction SilentlyContinue
prompt
$global:LASTEXITCODE = 0
Write-Output success
prompt
Set-Location Env:
prompt
'''
            result = subprocess.run([POWERSHELL, "-NoLogo", "-NoProfile", "-Command", script],
                                    env=env, capture_output=True, text=True, timeout=15)
            path = directory.as_posix()
            if not path.startswith("/"):
                path = "/" + path
            report = "\x1b]7;file://localhost" + quote(path, safe="/._~-:") + "\x07"
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stderr, "")
            self.assertEqual(result.stdout, report + "user-hook:False/17\nsuccess\n" +
                             report + "user-hook:True/0\nuser-hook:True/0\n")

    @unittest.skipUnless(POWERSHELL, "PowerShell is not installed")
    def test_powershell_opt_out(self):
        env = dict(os.environ, ETERM_TEST_INTEGRATION=str(INTEGRATION / "powershell.ps1"),
                   ETERM_SHELL_INTEGRATION="0")
        script = 'function global:prompt { "original" }; . $env:ETERM_TEST_INTEGRATION; prompt'
        result = subprocess.run([POWERSHELL, "-NoLogo", "-NoProfile", "-Command", script],
                                env=env, capture_output=True, text=True, timeout=15)
        self.assertEqual(result.stdout, "original\n")
        self.assertEqual(result.stderr, "")
        self.assertEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
