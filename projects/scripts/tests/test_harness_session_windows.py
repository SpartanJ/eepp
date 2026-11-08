"""PowerShell adapter isolation, runnable with portable pwsh on non-Windows hosts."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_harness_session_recovery import ASSETS, BINARY, environment

PWSH=os.environ.get('ETERM_TEST_PWSH') or shutil.which('pwsh') or shutil.which('powershell')


@unittest.skipUnless(PWSH and BINARY.exists(),'Build eterm and install PowerShell first')
class PowerShellRecovery(unittest.TestCase):
    def test_disabled_and_nested_shells_do_not_install_a_codex_function(self):
        for disabled in (True,False):
            with self.subTest(disabled=disabled),tempfile.TemporaryDirectory() as temp:
                env=environment(Path(temp))
                env['ETERM_HARNESS_EMULATOR_PID']='1'
                env['ADAPTER']=str(ASSETS/'harness-session.ps1')
                if disabled: env['ETERM_HARNESS_DIRECTORY']=''
                r=subprocess.run([PWSH,'-NoProfile','-Command',
                    '. $env:ADAPTER; if (Test-Path function:codex) { throw "injected" }; exit 0'],
                    env=env,capture_output=True,text=True,timeout=10)
                self.assertEqual(r.returncode,0,r.stderr)
