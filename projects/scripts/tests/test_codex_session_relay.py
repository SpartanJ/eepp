"""Black-box transport/selection checks against the native CLI, without accounts or model calls."""
import json
import os
from pathlib import Path
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from test_harness_session_recovery import BINARY, SESSION, descriptor, environment

OTHER=SESSION[:-1]+'0'


def frame(value,masked=False,opcode=1,final=True):
    data=json.dumps(value).encode() if isinstance(value,dict) else value
    size=len(data)
    header=bytes([(128 if final else 0)|opcode,(128 if masked else 0)|(size if size<126 else 126 if size<65536 else 127)])
    if size>=126: header+=struct.pack('!H' if size<65536 else '!Q',size)
    if masked:
        mask=b'abcd'
        return header+mask+bytes(c^mask[i%4] for i,c in enumerate(data))
    return header+data


def exact(sock,count):
    value=b''
    while len(value)<count:
        chunk=sock.recv(count-len(value))
        if not chunk: raise EOFError('closed transport')
        value+=chunk
    return value


def message(sock):
    result=b''
    while True:
        first,second=exact(sock,2);size=second&127
        if size==126: size=struct.unpack('!H',exact(sock,2))[0]
        if size==127: size=struct.unpack('!Q',exact(sock,8))[0]
        mask=exact(sock,4) if second&128 else None
        data=exact(sock,size)
        if mask: data=bytes(c^mask[i%4] for i,c in enumerate(data))
        if first&15>=8: continue
        result+=data
        if first&128: return json.loads(result)


def header(sock):
    data=b''
    while not data.endswith(b'\r\n\r\n'): data+=exact(sock,1)
    return data


def mock_client():
    """The fixture only talks to its test server; it never launches an actual Codex process."""
    args=sys.argv[1:]
    if args==['app-server','daemon','start']:
        root=Path(os.environ['TEST_RECORD_DIRECTORY'])
        with (root/'daemon-started').open('a') as log: log.write('start\n')
        deadline=time.monotonic()+5
        while not (root/'daemon-ready').exists() and time.monotonic()<deadline: time.sleep(.01)
        assert (root/'daemon-ready').exists()
        return
    if not args or args[0]!='--remote' or not os.environ.get('ETERM_CODEX_RELAY_TOKEN'):
        print(json.dumps(args),flush=True)
        return
    remote=args[1]
    from urllib.parse import urlparse
    parsed=urlparse(remote);endpoint=(parsed.hostname,parsed.port)
    probe=socket.socket(socket.AF_INET,socket.SOCK_STREAM);probe.connect(endpoint);probe.close()
    def connect():
        sock=socket.socket(socket.AF_INET,socket.SOCK_STREAM);sock.settimeout(5);sock.connect(endpoint)
        sock.sendall(b'GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nAuthorization: Bearer '+os.environ['ETERM_CODEX_RELAY_TOKEN'].encode()+b'\r\n\r\n')
        assert header(sock).split(b'\r\n')[0].split()[1]==b'101'
        return sock
    first=connect()
    value=json.dumps({'id':1,'method':'thread/resume','params':{'threadId':SESSION}}).encode()
    # Exercise fragmented, masked requests with an interleaved control frame.
    first.sendall(frame(value[:11],True,final=False)+frame(b'ping',True,opcode=9)+frame(value[11:],True,opcode=0))
    assert message(first)['result']['thread']['id']==SESSION
    active=Path(os.environ['TEST_RECORD_DIRECTORY'])/'active.json'
    assert json.loads(active.read_text())['session_id']==SESSION
    second=connect()
    # A server request with ID 1 must not consume the pending client request with ID 1.
    second.sendall(frame({'id':1,'method':'thread/resume','params':{'threadId':'missing'}},True))
    approval=message(second)
    assert approval['method']=='item/commandExecution/requestApproval'
    assert active.read_text()==''
    second.sendall(frame({'id':1,'result':{'decision':'decline'}},True))
    assert 'error' in message(second)
    # Failed selection on B restores A's confirmed conversation, not B's empty local state.
    assert json.loads(active.read_text())['session_id']==SESSION
    second.sendall(frame({'id':2,'method':'thread/fork','params':{'threadId':OTHER}},True))
    assert message(second)['result']['thread']['id']==OTHER
    assert message(first)['result']['thread']['id']==SESSION
    assert json.loads(active.read_text())['session_id']==OTHER
    second.sendall(frame({'id':3,'method':'thread/start','params':{'ephemeral':True}},True))
    assert message(second)['result']['thread']['ephemeral']
    assert active.read_text()==''
    second.sendall(frame({'id':4,'method':'thread/resume','params':{'threadId':OTHER}},True))
    assert message(second)['result']['thread']['id']==OTHER
    approval=message(second)
    assert approval['method']=='item/commandExecution/requestApproval'
    second.sendall(frame({'id':7,'result':{'decision':'decline'}},True))
    # Observe bounded metadata without truncating the actual transport or poisoning later RPCs.
    # An unobservable outcome saves no session: the daemon may already have switched threads.
    second.sendall(frame({'id':5,'method':'thread/resume','params':{'threadId':'oversized'}},True))
    assert len(message(second)['result']['padding'])==8*1024*1024
    assert active.read_text()==''
    def select(request_id,selected):
        second.sendall(frame({'id':request_id,'method':'thread/resume','params':{'threadId':selected}},True))
        assert message(second)['result']['thread']['id']==selected
        assert json.loads(active.read_text())['session_id']==selected
    select(6,SESSION)
    def interrupted():
        sock=connect()
        sock.sendall(frame({'id':1,'method':'thread/resume','params':{'threadId':'interrupted'}},True))
        assert message(sock)['method']=='fixture/selectionPending'
        assert active.read_text()==''
        return sock
    # A selection interrupted by closing its connection is equally unconfirmed.
    pending=interrupted();pending.close()
    time.sleep(.15)
    assert active.read_text()==''
    pending=interrupted()
    select(8,OTHER)
    pending.close()
    # Closing an older connection cannot roll back a newer confirmed selection.
    time.sleep(.15)
    assert json.loads(active.read_text())['session_id']==OTHER
    # Overlapping selections: the older one succeeds after the newer was requested, then the newer
    # fails. The daemon is on the older thread, so the confirmed record from before both is stale.
    second.sendall(frame({'id':9,'method':'thread/resume','params':{'threadId':SESSION}},True)+
                   frame({'id':10,'method':'thread/resume','params':{'threadId':'fails'}},True))
    assert message(second)['result']['thread']['id']==SESSION
    assert 'error' in message(second)
    assert active.read_text()==''
    select(11,OTHER)
    # An outgoing request too large to observe may be an uncaptured selection.
    second.sendall(frame({'id':12,'method':'thread/resume',
                          'params':{'threadId':SESSION,'padding':'x'*(8*1024*1024)}},True))
    assert message(second)['result']['thread']['id']==SESSION
    assert active.read_text()==''
    select(13,OTHER)
    def delayed():
        sock=connect()
        sock.sendall(frame({'id':1,'method':'thread/resume','params':{'threadId':'delayed'}},True))
        assert message(sock)['method']=='fixture/selectionPending'
        return sock
    def release(sock):
        sock.sendall(frame({'id':98,'method':'fixture/release'},True))
        assert message(sock)['result']['thread']['id']==SESSION
        sock.close()
    # A newer error cannot restore the confirmed record while an older selection is unresolved:
    # the older one may already have switched threads.
    older=delayed()
    second.sendall(frame({'id':14,'method':'thread/resume','params':{'threadId':'fails'}},True))
    assert 'error' in message(second)
    assert active.read_text()==''
    release(older)
    assert active.read_text()==''
    select(15,OTHER)
    # A late older success clears a newer saved record at once, even with another one unresolved.
    older=delayed();unresolved=delayed()
    select(20,SESSION)
    release(older)
    assert active.read_text()==''
    release(unresolved)
    assert active.read_text()==''
    select(21,OTHER)
    # A lost request is an unseen newer selection: an earlier pending one cannot be saved after it.
    older=delayed()
    second.sendall(frame({'id':16,'method':'thread/resume',
                          'params':{'threadId':SESSION,'padding':'x'*(8*1024*1024)}},True))
    assert message(second)['result']['thread']['id']==SESSION
    release(older)
    assert active.read_text()==''
    select(17,OTHER)
    # A connection whose requests cannot be decoded (reserved bits) blocks saving until it closes.
    blind=connect()
    request=frame({'id':1,'method':'thread/resume','params':{'threadId':'blind'}},True)
    blind.sendall(bytes([request[0]|0x40])+request[1:])
    assert message(blind)['result']['thread']['id']==OTHER
    assert active.read_text()==''
    second.sendall(frame({'id':18,'method':'thread/resume','params':{'threadId':SESSION}},True))
    assert message(second)['result']['thread']['id']==SESSION
    assert active.read_text()==''
    blind.close()
    time.sleep(.15)
    select(19,OTHER)
    if os.environ.get('EXPECT_RESUME'):
        assert args[-2:]==['resume',SESSION]
        assert not os.environ.get('ETERM_RESUME_SESSION')
    print('READY',flush=True)
    sys.stdin.readline()
    first.close();second.close()


@unittest.skipUnless(BINARY.exists() and os.name!='nt' and hasattr(socket,'AF_UNIX'),
                     'Native host fixture needs a built eterm and POSIX executable scripts')
class CodexRelay(unittest.TestCase):
    def make_cli(self,root):
        executable=root/'codex'
        executable.write_text('#!'+sys.executable+'\nfrom test_codex_session_relay import mock_client\nmock_client()\n')
        executable.chmod(0o700)
        env=environment(root)
        env.update(PATH=str(root)+os.pathsep+os.environ['PATH'],
                   PYTHONPATH=str(Path(__file__).parent),CODEX_HOME=str(root),TEST_RECORD_DIRECTORY=str(root))
        return env

    def test_standalone_and_one_shot_bypass_preserves_argv(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);env=self.make_cli(root)
            for args in (['--no-daemon'],['resume',SESSION,'--no-daemon'],['exec','a space'],
                         ['--remote','unix:///custom.sock'],['--help'],
                         ['-c','model=fixture','exec','a space'],
                         ['--config=model=fixture','review'],['future-subcommand'],
                         ['--future-option','value']):
                r=subprocess.run([str(BINARY),'--harness-session','codex','--',*args],
                    env=env,capture_output=True,text=True,timeout=5)
                self.assertEqual(r.returncode,0,r.stderr)
                self.assertEqual(json.loads(r.stdout),args)

    def test_transport_probes_fragmentation_connections_approvals_and_capture(self):
        self.transport(False)

    def test_daemon_resume_uses_exact_id_and_same_transport(self):
        self.transport(True)

    def test_stale_default_socket_starts_daemon_and_retries_once(self):
        self.transport(False,stale=True)

    def transport(self,resume,stale=False):
        with tempfile.TemporaryDirectory(dir='/tmp') as temp:
            root=Path(temp);env=self.make_cli(root);upstream=root/'server.sock'
            if stale:
                upstream=root/'app-server-control'/'app-server-control.sock'
                upstream.parent.mkdir()
                previous=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM)
                previous.bind(str(upstream));previous.close()
            else:
                env['ETERM_CODEX_SOCKET']=str(upstream)
            server=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM)
            if not stale: server.bind(str(upstream));server.listen(8)
            server.settimeout(.2);stopped=threading.Event();failures=[];clients=[];threads=[]
            approved=threading.Event();first_socket=[]
            def serve(sock):
                sock.settimeout(5)
                try:
                    try: hello=header(sock)
                    except EOFError: return # Codex's closed endpoint probe.
                    self.assertIn(b'Upgrade: websocket',hello)
                    # The relay's token authorizes only the relay.
                    self.assertNotIn(b'authorization',hello.lower())
                    sock.sendall(b'HTTP/1.1 101\r\nUpgrade: websocket\r\n\r\n')
                    request=message(sock);selected=request['params']['threadId']
                    def response(request,selected,ephemeral=False):
                        return {'id':request['id'],'result':{'thread':{'id':selected,'sessionId':'wrong-root',
                            'cwd':temp,'path':str(root/'transcript'),'ephemeral':ephemeral}}}
                    if selected=='interrupted':
                        sock.sendall(frame({'id':99,'method':'fixture/selectionPending'}))
                    elif selected=='delayed':
                        sock.sendall(frame({'id':99,'method':'fixture/selectionPending'}))
                        message(sock)
                        sock.sendall(frame(response(request,SESSION)))
                    elif selected=='blind':
                        sock.sendall(frame(response(request,OTHER)))
                    elif selected==SESSION:
                        result=response(request,selected)
                        data=json.dumps(result).encode()
                        sock.sendall(frame(data[:9],final=False)+frame(b'ping',opcode=9)+frame(data[9:],opcode=0))
                        first_socket.append((sock,result))
                    else:
                        self.assertEqual(selected,'missing')
                        sock.sendall(frame({'id':1,'method':'item/commandExecution/requestApproval'}))
                        self.assertEqual(message(sock),{'id':1,'result':{'decision':'decline'}})
                        sock.sendall(frame({'id':1,'error':{'code':-1,'message':'unavailable'}}))
                        request=message(sock)
                        sock.sendall(frame(response(request,OTHER)))
                        first_socket[0][0].sendall(frame(first_socket[0][1]))
                        request=message(sock)
                        sock.sendall(frame(response(request,OTHER,True)))
                        request=message(sock)
                        sock.sendall(frame(response(request,OTHER)))
                        sock.sendall(frame({'id':7,'method':'item/commandExecution/requestApproval'}))
                        self.assertEqual(message(sock),{'id':7,'result':{'decision':'decline'}})
                        approved.set()
                        while not stopped.is_set():
                            request=message(sock)
                            selected=request['params']['threadId']
                            if selected=='fails':
                                sock.sendall(frame({'id':request['id'],'error':{'code':-1,'message':'unavailable'}}))
                                continue
                            result=response(request,OTHER if selected=='oversized' else selected)
                            if selected=='oversized': result['result']['padding']='x'*(8*1024*1024)
                            sock.sendall(frame(result))
                    stopped.wait(8)
                except EOFError:
                    pass
                except Exception as error:
                    if not stopped.is_set(): failures.append(error)
            def accept():
                if stale:
                    while not stopped.is_set() and not (root/'daemon-started').exists():
                        time.sleep(.01)
                    if stopped.is_set(): return
                    upstream.unlink();server.bind(str(upstream));server.listen(8)
                    (root/'daemon-ready').touch()
                while not stopped.is_set():
                    try: sock,_=server.accept()
                    except socket.timeout: continue
                    except OSError: break
                    clients.append(sock)
                    worker=threading.Thread(target=serve,args=(sock,));threads.append(worker);worker.start()
            accepting=threading.Thread(target=accept);accepting.start()
            (root/'transcript').write_text('fixture')
            if resume:
                record=descriptor(root,mode='daemon');record['socket']=str(upstream)
                env.update(ETERM_RESUME_SESSION=json.dumps(record),EXPECT_RESUME='1')
            command=[str(BINARY),'--harness-session',*(['resume'] if resume else ['codex','--'])]
            child=subprocess.Popen(command,env=env,
                stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            try:
                ready=child.stdout.readline().strip()
                self.assertEqual(ready,'READY',child.stderr.read() if not ready else '')
                self.assertTrue(approved.wait(2))
                saved=json.loads((root/'active.json').read_text())
                self.assertEqual((saved['session_id'],saved['mode'],saved['owner_pid']),
                                 (OTHER,'daemon',child.pid))
                self.assertEqual(Path(saved['socket']),upstream)
                os.kill(child.pid,signal.SIGINT)
                self.assertIsNone(child.poll())
                child.stdin.write('exit\n');child.stdin.flush()
                out,err=child.communicate(timeout=5)
                self.assertEqual(child.returncode,0,err)
                self.assertEqual(out,'')
                self.assertFalse(failures,failures)
                if stale: self.assertEqual((root/'daemon-started').read_text(),'start\n')
            finally:
                if child.poll() is None: child.kill();child.communicate()
                stopped.set();server.close();accepting.join(2)
                for sock in clients: sock.close()
                for worker in threads: worker.join(2)
