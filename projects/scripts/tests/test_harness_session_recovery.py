"""Black-box checks for eterm's native recovery CLI and its thin shell adapters."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
ASSETS = ROOT / 'bin/assets/shell-integration'
BINARY = Path(os.environ.get('ETERM_TEST_BINARY', ROOT / 'bin/eterm')).resolve()
SESSION = '019a1234-1234-7123-8123-123456789abc'


def environment(directory):
    env=dict(os.environ, ETERM_HARNESS_DIRECTORY=str(directory),
             ETERM_HARNESS_BINARY=str(BINARY), ETERM_HARNESS_SHELL_PID=str(os.getpid()))
    # Claude exports its mode to hooks; a run inside Claude must not leak its own into fixtures.
    env.pop('CLAUDE_CODE_ENTRYPOINT',None)
    return env


def descriptor(directory, kind='codex', mode='standalone'):
    return dict(version=1, harness=kind, session_id=SESSION, cwd=str(directory),
                transcript=str(directory / 'transcript'), home='', mode=mode)


@unittest.skipUnless(BINARY.exists(), 'Build eterm first')
class HarnessRecovery(unittest.TestCase):
    @unittest.skipUnless(sys.platform=='linux' and shutil.which('fish'),
                         'Executable replacement regression needs Linux /proc and fish')
    def test_new_tabs_after_executable_replacement(self):
        for operation in ('replace','rename','remove'):
            with self.subTest(operation=operation),tempfile.TemporaryDirectory() as temp:
                root=Path(temp).resolve();bindir=root/'bin';bindir.mkdir()
                executable=bindir/'eterm';shutil.copy2(BINARY,executable)
                (bindir/'assets').symlink_to(ROOT/'bin/assets',target_is_directory=True)
                (root/'libs').symlink_to(ROOT/'libs',target_is_directory=True)
                profile=root/'profile';profile.mkdir();records=root/'records';records.mkdir()
                (profile/'config.cfg').write_text('[terminal]\nshell='+shutil.which('fish')+
                    '\nrecover_harness_sessions=true\nrestore_last_session=false\n')
                writer=root/'writer.py'
                writer.write_text('import os,json\nfrom pathlib import Path\n'
                    'directory=os.getenv("ETERM_HARNESS_DIRECTORY","")\n'
                    'manifest=Path(directory,".claude-plugin/plugin.json")\n'
                    'plugin=json.loads(manifest.read_text()) if directory and manifest.exists() else None\n'
                    'value=dict(helper=os.getenv("ETERM_HARNESS_BINARY",""),directory=directory,'
                    'shell=os.getenv("ETERM_HARNESS_SHELL_PID",""),plugin=plugin)\n'
                    'Path(os.environ["ETERM_TEST_RECORDS"],str(os.getppid())+".json").write_text(json.dumps(value))\n')
                env=dict(os.environ,ETERM_CONFIG_PATH=str(profile),EEPP_RUNTIME='headless',
                    EEPP_INSPECTOR='1',EEPP_INSPECTOR_PORT='0',
                    EEPP_INSPECTOR_TOKEN='eterm-rebuild-regression',
                    ETERM_TEST_RECORDS=str(records),ETERM_TEST_WRITER=str(writer),
                    ETERM_TEST_PYTHON=sys.executable)
                for key in tuple(env):
                    if key.startswith('ETERM_HARNESS_') or key in (
                            'ETERM_RESUME_SESSION','ETERM_SHELL_INTEGRATION','CLAUDE_CODE_PLUGIN_DIRS'):
                        env.pop(key,None)
                log_path=root/'log';log=log_path.open('w')
                child=subprocess.Popen([str(executable),'--shell-args=--no-config\n--init-command\n'
                    'command "$ETERM_TEST_PYTHON" "$ETERM_TEST_WRITER"'],
                    env=env,stdout=log,stderr=log)
                inspector=None
                def wait_for(predicate):
                    deadline=time.monotonic()+10
                    while time.monotonic()<deadline and child.poll() is None:
                        if predicate(): return
                        time.sleep(.02)
                    self.fail('Timed out waiting for eterm: '+log_path.read_text()[-2000:])
                def key(value):
                    result=subprocess.run([sys.executable,str(ROOT/'projects/scripts/eepp-inspect.py'),
                        '--port',str(inspector['port']),'--token',env['EEPP_INSPECTOR_TOKEN'],
                        'raw',json.dumps(dict(method='input.key',params=dict(key=value,
                            modifiers=['Ctrl','Shift'])))],capture_output=True,text=True,timeout=5)
                    if child.poll() is None:
                        self.assertEqual(result.returncode,0,result.stderr)
                try:
                    wait_for(lambda: len(list(records.glob('*.json')))==1)
                    for line in log_path.read_text().splitlines():
                        if line.startswith('EEPP_INSPECTOR='):
                            inspector=json.loads(line.split('=',1)[1]);break
                    self.assertIsNotNone(inspector)
                    initial=next(records.glob('*.json'))
                    self.assertEqual(json.loads(initial.read_text())['helper'],str(executable))
                    if operation=='remove':
                        executable.unlink()
                    else:
                        replacement=bindir/'replacement';shutil.copy2(BINARY,replacement)
                        if operation=='rename':
                            previous=bindir/'old-eterm';executable.rename(previous)
                        os.replace(replacement,executable)
                        if operation=='rename': previous.unlink()
                    self.assertTrue(os.readlink('/proc/'+str(child.pid)+'/exe').endswith(' (deleted)'))
                    key('t')
                    wait_for(lambda: len(list(records.glob('*.json')))==2)
                    latest=next(p for p in records.glob('*.json') if p!=initial)
                    value=json.loads(latest.read_text())
                    if operation=='remove':
                        self.assertEqual((value['helper'],value['directory'],value['shell']),('','',''))
                        self.assertIsNone(value['plugin'])
                    else:
                        self.assertEqual(value['helper'],str(executable))
                        self.assertEqual(value['shell'],latest.stem)
                        self.assertEqual(value['plugin']['hooks']['SessionStart'][0]['hooks'][0]['command'],
                                         str(executable))
                finally:
                    if inspector:
                        for _ in range(2):
                            if child.poll() is not None: break
                            try: key('w')
                            except (subprocess.TimeoutExpired,AssertionError): break
                    try: child.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        child.terminate();child.wait(timeout=5)
                    log.close()
                self.assertEqual(child.returncode,0)
                for path in records.glob('*.json'):
                    directory=json.loads(path.read_text())['directory']
                    if directory: self.assertFalse(Path(directory).exists())

    def test_cli_dispatch_and_disabled_hook_are_silent_without_app_configuration(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            env = dict(os.environ, ETERM_HARNESS_DIRECTORY='', ETERM_CONFIG_PATH=str(root/'unused'))
            for payload in (b'', b'not json', b'x'*65537):
                result = subprocess.run([str(BINARY), '--harness-session', 'hook', 'codex'],
                    input=payload, env=env, capture_output=True, timeout=5)
                self.assertEqual((result.returncode,result.stdout,result.stderr), (0,b'',b''))
            result = subprocess.run([str(BINARY), '--harness-session', 'unknown'],
                env=env,capture_output=True,timeout=5)
            self.assertEqual(result.returncode,2)
            self.assertFalse((root/'unused').exists())

    def test_resume_rejects_invalid_ids_paths_and_missing_transcripts(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); (root/'transcript').write_text('fixture')
            good=descriptor(root)
            bad=[dict(good,session_id='latest'),dict(good,cwd='relative'),
                 dict(good,cwd=str(root)+'\x1b'),dict(good,transcript=str(root/'missing')),
                 dict(good,harness='unknown'),dict(good,version='one')]
            for record in bad:
                env=environment(root); env['ETERM_RESUME_SESSION']=json.dumps(record)
                r=subprocess.run([str(BINARY),'--harness-session','resume'],env=env,
                    capture_output=True,text=True,timeout=5)
                self.assertEqual(r.returncode,0,r.stderr)
                self.assertIn('unavailable',r.stderr)

    def test_foreign_hook_reports_cannot_overwrite_metadata(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); path=root/'active.json'; path.write_text('untouched')
            env=environment(root)
            payload=dict(hook_event_name='SessionStart',session_id=SESSION,
                         cwd=temp,transcript_path=str(root/'transcript'))
            for value in (payload,dict(payload,agent_id='nested'),None):
                r=subprocess.run([str(BINARY),'--harness-session','hook','codex'],env=env,
                    input=json.dumps(value),capture_output=True,text=True,timeout=5)
                self.assertEqual((r.returncode,r.stdout,r.stderr),(0,'',''))
                self.assertEqual(path.read_text(),'untouched')

    @unittest.skipUnless(shutil.which('node'), 'npm launcher fixture requires Node.js')
    def test_npm_harness_hooks_capture_without_wrapper_and_reject_one_shots(self):
        for kind,entry in (('codex','@openai/codex/bin/codex.js'),
                           ('claude','@anthropic-ai/claude-code/cli.js')):
            with self.subTest(kind=kind),tempfile.TemporaryDirectory() as temp:
                root=Path(temp);script=root/'node_modules'/entry
                script.parent.mkdir(parents=True)
                script.write_text("if (process.env.TEST_TITLE) process.title = process.env.TEST_TITLE;\n"
                    "const cp = require('node:child_process');\n"
                    "const result = cp.spawnSync(process.env.ETERM_HARNESS_BINARY, "
                    "['--harness-session','hook',process.env.TEST_KIND], "
                    "{input:process.env.TEST_PAYLOAD,encoding:'utf8'});\n"
                    "process.stdout.write(result.stdout);process.stderr.write(result.stderr);"
                    "process.exit(result.status);\n")
                env=environment(root);env.update(TEST_KIND=kind,TEST_PAYLOAD=json.dumps(dict(
                    hook_event_name='SessionStart',session_id=SESSION,cwd=temp,
                    transcript_path=str(root/'transcript'))))
                cases=[([],True),(['exec','task'] if kind=='codex' else ['--print','task'],False)]
                if kind=='codex':
                    # A whitespace word cannot be a subcommand: it is an initial TUI prompt.
                    cases+=[(['fix the build'],True),(['task'],False),
                            (['fix the build','-m','model'],True),
                            (['fix the build','--remote','ws://localhost:1234'],False),
                            (['fix the build','--help'],False),(['fix the build','resume'],False)]
                # npm puts a symlink to the entry point on PATH, and node reports that path.
                link=root/'bin'/kind;link.parent.mkdir();link.symlink_to(script)
                runs=[(args,expected,script,{}) for args,expected in cases]
                runs+=[([],True,link,{}),(cases[1][0],False,link,{})]
                if kind=='claude':
                    # npm Claude's process title overwrites its visible argv; its exported
                    # entry point still tells the interactive TUI from --print.
                    runs+=[([],True,script,dict(TEST_TITLE='claude',CLAUDE_CODE_ENTRYPOINT='cli')),
                           (['--print','task'],False,script,
                            dict(TEST_TITLE='claude',CLAUDE_CODE_ENTRYPOINT='sdk-cli'))]
                for args,expected,entry,extra in runs:
                    path=root/'active.json';path.write_text('untouched')
                    result=subprocess.run([shutil.which('node'),str(entry),*args],env=dict(env,**extra),
                                          capture_output=True,text=True,timeout=10)
                    self.assertEqual((result.returncode,result.stdout,result.stderr),(0,'',''))
                    if expected:
                        self.assertEqual(json.loads(path.read_text())['session_id'],SESSION,(args,entry,extra))
                    else:
                        self.assertEqual(path.read_text(),'untouched',(args,entry,extra))

    @unittest.skipIf(os.name=='nt','Host executable-script fixtures use POSIX shebangs')
    def test_shells_resume_exact_argv_once_and_clear_at_prompt(self):
        shells=[p for p in ('/bin/bash',shutil.which('zsh'),shutil.which('fish')) if p and Path(p).exists()]
        pwsh=os.environ.get('ETERM_TEST_PWSH') or shutil.which('pwsh')
        if pwsh: shells.append(pwsh)
        for shell in shells:
            for kind in ('codex','claude'):
                with self.subTest(shell=shell,kind=kind),tempfile.TemporaryDirectory() as temp:
                    root=Path(temp); work=root/"space ' é [literal]";work.mkdir()
                    (root/'transcript').write_text('fixture')
                    executable=root/kind
                    executable.write_text('#!'+sys.executable+'\nimport os,sys,json\nfrom pathlib import Path\n'
                        'with open(os.environ["CALLS"],"a") as f: f.write(json.dumps([sys.argv[1:],os.getcwd(),os.getenv("ETERM_RESUME_SESSION"),os.getenv("CODEX_HOME"),os.getenv("CLAUDE_CONFIG_DIR")])+"\\n")\n'
                        'Path(os.environ["ETERM_HARNESS_DIRECTORY"],"active.json").write_text("active")\nsys.exit(7)\n')
                    executable.chmod(0o700)
                    record=descriptor(root,kind);record['cwd']=str(work);record['home']=str(root/'home')
                    env=environment(root);env.update(PATH=temp+os.pathsep+os.environ['PATH'],
                        ETERM_HARNESS_EMULATOR_PID=str(os.getpid()),ETERM_RESUME_SESSION=json.dumps(record),CALLS=str(root/'calls'))
                    name=Path(shell).name
                    if name=='fish':
                        asset=ASSETS/'harness-session.fish'
                        script='source "$ADAPTER"; source "$ADAPTER"; __eterm_clear_harness_session'
                        args=[shell,'--no-config','-c',script]
                    elif name in ('pwsh','powershell'):
                        asset=ASSETS/'harness-session.ps1'
                        args=[shell,'-NoLogo','-NoProfile','-Command',
                              '. $env:ADAPTER; . $env:ADAPTER; __etermClearHarness']
                    else:
                        asset=ASSETS/'harness-session.sh'
                        args=[shell,'-c','source "$COMMON"; source "$ADAPTER"; source "$ADAPTER"; __eterm_clear_harness_session']
                    env['ADAPTER']=str(asset)
                    env['COMMON']=str(ASSETS/'directory-reporting.sh')
                    r=subprocess.run(args,env=env,capture_output=True,text=True,timeout=10)
                    self.assertEqual(r.returncode,0,r.stderr)
                    self.assertIn('status 7',r.stderr)
                    calls=[json.loads(line) for line in (root/'calls').read_text().splitlines()]
                    self.assertEqual(len(calls),1)
                    self.assertEqual(calls[0][0],['resume','--no-daemon',SESSION] if kind=='codex' else ['--resume',SESSION])
                    self.assertEqual(Path(calls[0][1]).resolve(),work.resolve())
                    self.assertFalse(calls[0][2])
                    self.assertEqual(calls[0][3 if kind=='codex' else 4],record['home'])
                    self.assertEqual((root/'active.json').read_text(),'')
