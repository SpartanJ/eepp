"""Prompt behavior, including Bash versions that parse arrays but execute only element zero."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_harness_session_recovery import ASSETS


class PromptHooks(unittest.TestCase):
    def test_noclobber_status_and_existing_prompt_command(self):
        shells = [p for p in ('/bin/bash', shutil.which('bash'), shutil.which('zsh')) if p]
        for shell in dict.fromkeys(shells):
            with self.subTest(shell=shell), tempfile.TemporaryDirectory() as temp:
                env = dict(os.environ, ETERM_HARNESS_DIRECTORY=temp,
                           ETERM_HARNESS_BINARY=shutil.which('true'), COMMON=str(ASSETS/'directory-reporting.sh'),
                           ADAPTER=str(ASSETS/'harness-session.sh'))
                # Real interactive prompt execution catches Bash 5.0's array behavior; manually
                # eval'ing PROMPT_COMMAND would hide exactly that regression.
                if Path(shell).name == 'zsh':
                    script = '''source "$COMMON"
source "$ADAPTER"
printf live >| "$ETERM_HARNESS_DIRECTORY/active.json"
set -C
false
__eterm_clear_harness_session
printf 'status:%d\\n' "$?"
[[ ! -s $ETERM_HARNESS_DIRECTORY/active.json ]]
'''
                    args = [shell, '-f', '-c', script]
                    result = subprocess.run(args, input='', env=env, capture_output=True, text=True, timeout=5)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertIn('status:1', result.stdout)
                    continue
                script = '''PS1=''
PROMPT_COMMAND='printf "user-status:%d\\n" "$?"'
source "$COMMON"
__eterm_add_prompt_hook __eterm_report_working_directory
source "$ADAPTER"
printf live >| "$ETERM_HARNESS_DIRECTORY/active.json"
set -C
false
exit 0
'''
                result = subprocess.run([shell, '--noprofile', '--norc', '-i'], input=script,
                    env=env, capture_output=True, text=True, timeout=5)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn('user-status:1', result.stdout)
                self.assertIn('\x1b]7;file://localhost', result.stdout)
                self.assertEqual((Path(temp)/'active.json').read_text(), '')
