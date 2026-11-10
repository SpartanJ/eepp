# eterm harness session recovery investigation

Investigated 2026-11-06. The research below preceded the optional Codex/Claude prototype described
in the [integration guide](../articles/eterm_integration.md#experimental-codex-and-claude-session-recovery).
It is a best-effort convenience, not a promise of general session recovery. Here, `omp` means
can1357/oh-my-pi and `pi` means the coding agent
formerly hosted at badlogic/pi-mono, now earendil-works/pi.

## Finding

eterm can reopen a pane into its specific saved coding-agent conversation. The practical mechanism
is to retain the harness's session identity and invoke its supported resume operation. The hard
part is reliably associating the **currently selected** conversation with the terminal pane.

This should promise recovery to the harness's last durable conversation state. It cannot generally
promise the exact pre-close state: partial streaming output, unsent input, live subprocesses,
pending approvals, temporary UI state, and extension memory depend on each harness's persistence.
Restoring conversation history also does not roll back files or recreate a deleted worktree.

Keeping a process and its PTY alive behind a detached backend is a separate option for preserving
live execution across UI closure. That is a considerably larger lifecycle change and still does
not survive backend termination or reboot without application-specific persistence.

## Existing eterm boundaries

- `src/tools/eterm/eterm.cpp`: `App::registerSessionWidgetType()` saves a custom title and working
  directory per terminal. Its loader calls `newTerminal()` with a directory and starts a new shell.
  This is the natural location for an optional structured resume descriptor in each pane's JSON.
- `App::saveSession()` serializes the complete splitter tree; `App::restoreSession()` reconstructs
  it. Explicit command/directory launches bypass normal restoration. Preserve those semantics.
- `src/tools/eterm/appconfig.cpp`: `AppConfig::takeSession()` reads and removes `session.json`;
  `saveSession()` writes it directly. It is currently a consumed window-close snapshot, not a
  continuously updated crash-recovery journal. Recovering after a crash needs additional persistence.
- `App::closeWindow()` saves before terminal destruction. Snapshot the active resume association
  before shutdown hooks can clear it; distinguish closing eterm from voluntarily exiting a harness.
- `App::newTerminal()` passes an empty environment override map to `TerminalDisplay::create()`;
  the existing launch path can pass pane identity and a metadata endpoint through that map.
- `TerminalSession` owns the worker; UI reads immutable snapshots. `TerminalSnapshot` already
  carries cwd, the launched process ID, alternate-screen state and prompt state. Filesystem discovery
  and process inspection should stay outside the render path and VT parser.
- `TerminalDisplay::getProcessId()` identifies the launched process, usually the shell, not
  necessarily the foreground harness. `App::hasRunningChildren()` only asks whether children exist.
  It supplies neither foreground identity nor a harness session ID.
- Existing shell integration reports OSC 7; OSC 133 parsing also exists. Neither reports a stable
  conversation ID. `ProgramStatusTracker` handles transient OSC 7501 status records; those IDs
  identify status records and must not be interpreted as resumable conversations.

## Harness support

Commands below are intended interactive resume forms, with placeholders passed as separate argv
elements. Do not append the original prompt or substitute a most-recent-session selector.

| Harness | Exact target | How eterm could learn the target | Assessment |
| --- | --- | --- | --- |
| Codex CLI | `codex resume <session-id>` | `SessionStart` hook supplies `session_id`, cwd and optional transcript path | Strong first candidate; hook installation/trust and installed-version support need validation |
| Claude Code | `claude --resume <session-id>` | `SessionStart` hook supplies session identity; refresh on conversation replacement | Strong first candidate; `--session-id <uuid>` can also assign identity to a new managed launch |
| pi | `pi --session <session-file-or-id>` | Extension reads `ctx.sessionManager.getSessionId()` / `getSessionFile()` at lifecycle events | Strong candidate; distinguish ephemeral sessions and session-tree position |
| OMP | `omp --resume <session-id-or-path>` | Extension lifecycle plus session manager; existing per-terminal breadcrumbs are a useful secondary source | Strong candidate; track session changes and storage/profile context |
| Gemini CLI | `gemini --resume <full-uuid>` | Lifecycle hooks supply session ID, transcript path and cwd | Good next candidate |
| OpenCode | `opencode --session <session-id>` for the documented v1 TUI | Version-specific TUI integration must report the selected session | Resume exists; pane association needs a prototype, especially with a shared server |
| GitHub Copilot CLI | `copilot --resume <session-id>` | Current ID is exposed in the CLI; automatic lifecycle capture needs separate verification | Resume verified; integration discovery remains open |
| Cursor CLI | `agent --resume=<chat-id>` in current CLI docs | Automatic ID capture and executable/version detection need separate verification | Resume verified; integration discovery remains open |
| Aider | `aider --chat-history-file <file> --restore-chat-history` | Track an explicit history file and relevant launch configuration | Partial restoration; history-file recovery is not an exact named session abstraction |

Codex's official [CLI reference](https://learn.chatgpt.com/docs/developer-commands?surface=cli)
documents exact-ID resume and cwd selection. Its [hooks reference](https://learn.chatgpt.com/docs/hooks)
documents session identity and explicitly says the transcript format is not stable. Prefer hooks
over parsing rollouts. Subagent hooks can carry the parent's session ID, so event type matters.

Claude's [CLI reference](https://code.claude.com/docs/en/cli-reference) documents resume and assigned
UUIDs. Its [hooks reference](https://code.claude.com/docs/en/hooks) provides lifecycle inputs. Capture
identity changes, rather than assuming the initial launch ID remains current forever.

pi's [session guide](https://github.com/earendil-works/pi/blob/main/packages/coding-agent/docs/sessions.md)
describes automatic persistence, working-directory behavior and ephemeral runs. Its
[extension API](https://github.com/earendil-works/pi/blob/main/packages/coding-agent/docs/extensions.md)
exposes identity and current leaf; current lifecycle documentation includes start reasons for new,
resume and fork. Validate branch restoration against the installed release before claiming it.

OMP's [CLI reference](https://github.com/can1357/oh-my-pi/blob/main/docs/cli-reference.md) distinguishes
`--resume` from `--restore`: the latter actively continues interrupted work. Use `--resume` by default.
The [session implementation guide](https://github.com/can1357/oh-my-pi/blob/main/docs/session.md)
describes terminal breadcrumbs, lazy persistence and ownership leases. A new session ID can exist
before its transcript is materialized; a live writer can cause a resumed copy to acquire another ID.
Its [extension guide](https://github.com/can1357/oh-my-pi/blob/main/docs/extensions.md) exposes session
lifecycle events and the session manager.

Importantly, OMP's [terminal-ID implementation](https://github.com/can1357/oh-my-pi/blob/main/packages/tui/src/ttyid.ts)
prefers the current TTY path over terminal environment variables. Merely setting `TERM_SESSION_ID`
in eterm will **not** make its ordinary POSIX breadcrumbs stable across PTY recreation. A breadcrumb
can help identify an old live terminal, but eterm must save the resolved session identity itself.

Gemini documents [exact UUID resume](https://geminicli.com/docs/cli/session-management/) and
[hook identity fields](https://geminicli.com/docs/hooks/reference/). OpenCode's
[v1 CLI reference](https://dev.opencode.ai/docs/cli) documents TUI session selection; current
[commands documentation](https://opencode.ai/docs/cli/commands/) also exposes session operations and
separate server management. A server event saying a session was created does not prove that a
particular TUI pane has selected it. Keep v1/v2 integrations distinct until tested.

Additional resume references: [Copilot](https://docs.github.com/en/copilot/how-tos/copilot-cli/use-copilot-cli/chronicle),
[Cursor](https://prod.cursor.com/docs/cli/overview), and
[Aider](https://aider.chat/docs/config/options.html). These are additional coverage candidates,
not a measured popularity ranking or verified automatic integrations.

## Recommended approach

Give each eterm pane a persistent UUID, with a fresh launch generation for each new shell. Inject
that identity and a local metadata endpoint into the pane's environment. Small harness hooks or
extensions report **data**: harness kind, session ID/path, harness cwd, instance identity, and
whether the session is actually persistent. The endpoint/identity names are still to be designed.

Prefer a local helper writing bounded records atomically into an eterm-owned runtime directory
for the first prototype. It can work from hooks whose stdout is reserved for JSON, without assuming
hook output reaches the PTY. Prove Windows permissions and atomic replacement behavior before
calling this portable. If latency or lifecycle handling warrants IPC later, use the existing
platform facilities; a network service is unnecessary for this feature.

Keep coordination in a small eterm application-owned class with matching header/source files.
`App` owns it and its teardown. Keep harness knowledge out of `TerminalEmulator`; it should not
inspect transcript formats or launch programs. Install integrations explicitly and compose with
existing hooks rather than replacing them. No harness configuration was changed in this investigation.

Track lifecycle deliberately:

1. A top-level interactive harness registers with pane UUID, launch generation and instance ID.
2. New/resume/fork events replace its current session association. Nested agents and harness tool
   subprocesses must not overwrite it merely because they inherited the environment.
3. A voluntary harness exit returns the pane to shell-only restoration. Use instance-matched cleanup
   so delayed exit events cannot erase a newer harness's registration. Missing shutdown hooks need
   reconciliation with process lifetime or a shell prompt hook; idle sessions must not expire by age.
4. Closing eterm freezes the pane's current association before terminating children. The persistent
   window snapshot owns that association even if later shutdown events remove runtime records.
5. Startup validates a saved descriptor, recreates the pane, and resumes the exact target. A new
   registration confirms which conversation the harness actually opened.

Only accept a report as an automatic resume candidate when its local top-level ownership is
established. An inherited pane ID alone cannot establish that ownership. Versioned records,
per-launch scope, process start identity and harness lifecycle filtering need to be tested together.
For ambiguous nested/background cases, retain shell restoration until the integration can resolve
them. This is the largest correctness requirement of the prototype.

A saved descriptor should contain a schema version, pane ID, recognized harness kind, opaque
session ID or path, actual harness cwd, and necessary non-secret profile/storage context. Separate
the last shell cwd from the harness cwd: a harness may have been started with its own directory
override or worktree. Retain a validated launch recipe only for fields the adapter understands.
Do not copy prompts, API keys, complete environments, or arbitrary hook-provided executable text.

The application constructs resume argv from the recognized harness kind. Never evaluate a string
from terminal output, status metadata, a title or a transcript as a shell command. An ordinary
program can print escape sequences; base64 encoding does not make such output trustworthy.

### Launch behavior needs a small experiment

Directly spawning `codex resume ...` is straightforward, but skips shell initialization and may
lose aliases, PATH changes, provider environment and the expected shell prompt after exit.
Automatically typing a command into a shell can race its startup or execute into the wrong program.

Prototype a shell-integration handshake: the newly launched ordinary shell signals readiness and
consumes a one-shot, structured resume request through a trusted helper. Keep argv boundaries intact
and return to that shell on harness exit. Verify Bash, zsh, fish and PowerShell separately. For a
custom shell without integration, offer direct executable launch or leave a visible resume action;
do not pretend a portable shell-ready mechanism already exists.

### Unknown programs and alternate screens

Restarting recognized `htop`/`btop` launches is reasonable. Alternate-screen mode alone cannot decide
whether a program is safe or useful to replay: editors and agent harnesses use it too, and some
agents support inline mode. Persist a known launch recipe or add command tracking plus a deliberately
small restart policy. Command tracking must account for shell quoting, pipelines, functions and
aliases; shell command text is not equivalent to argv.

Foreground-process inspection can identify an executable and sometimes its arguments, but cannot
generically recover an in-process session switch. POSIX foreground process groups and Windows
ConPTY require different handling. Avoid scanning the complete process table per pane per frame.
Inspect only on lifecycle events or bounded background work; do not infer identity from the newest
file in a harness's session directory. Two simultaneous panes in one cwd invalidate that inference.

### Failure and scope policy

- Restore exact IDs, never `--last`, `--continue`, a mutable session name, or a numbered picker row.
- On missing session, deleted worktree, unsupported version or missing executable, keep the pane
  usable and explain why resume failed. Do not silently select another conversation or start fresh
  under a label claiming successful recovery.
- Do not replay the original prompt, approval answers, or flags that create/fork a new session.
  Reconcile model/profile/permission options through the adapter and existing harness configuration.
- Handle already-live sessions according to harness ownership/attach behavior. Do not assume all
  tools refuse a second writer; OMP explicitly has copy/fork behavior under contention.
- Start with local, direct panes. SSH, containers, tmux and Zellij need their own identity/launch
  context; a remote session ID cannot be resumed against the local harness store.
- Crash recovery needs debounced atomic workspace checkpoints and unambiguous multi-window
  ownership; current consumed `session.json` semantics alone do not provide it.

## Suggested delivery and acceptance checks

Start with Codex and Claude, then pi and OMP, followed by Gemini. Treat OpenCode as a separate
prototype because backend session events may lack TUI selection identity. Broaden to Copilot,
Cursor and Aider after automatic identity capture is verified for their supported versions.

The first prototype succeeds only if two panes in the **same cwd** resume two distinct conversations,
including after one pane switches/forks its session. Also check normal harness exit followed by
closing eterm, a harness killed without hooks, nested agent tool invocations, lazy/ephemeral sessions,
missing targets, non-default homes/profiles, and contention with a still-running session. Verify
no original task is resubmitted and no alternate session is chosen on failure.

Validate the restored splits, focus and terminal UI with the existing inspector, using hidden native
windows on macOS. Identity and recovery checks need isolated harness storage and actual harness
runs; an inspector screenshot or a mocked command is insufficient evidence of conversation recovery.

## Evidence limits of the initial investigation

Read eterm's save/load, terminal launch, worker snapshot, process lifetime, shell integration and
status paths, plus upstream documentation/source linked above. Locally checked Codex CLI 0.167.0
and Claude Code 2.1.311 help for resume syntax. Gemini reports 0.38.2. OMP and pi were not installed.
The installed OpenCode link points to 1.14.48, but even its version invocation attempted a write to
its user data directory and was blocked by the filesystem sandbox; no broader access was requested.

No existing conversations were opened, read or modified. No paid model calls or harness lifecycle
experiments were performed. The design's session capture, shell handshake and exact restoration
behavior still require the focused prototype above. This documentation-only investigation does
not require a project build or unit-test run.

## Codex/Claude prototype follow-up

The prototype is opt-in through `recover_harness_sessions`, disabled by default. It supports ordinary
local fish, Bash and zsh launches on macOS/Linux. OpenCode remains deferred. The user-facing scope
and additive hook setup are in the integration guide; global harness hook configuration was not
edited automatically.

`App` owns `HarnessSessionRecovery` (`harnesssessionrecovery.hpp/.cpp`). Each opted-in terminal
gets a private `mkdtemp` runtime directory. Its SessionStart hook writes a bounded, atomically
replaced descriptor; the application reads it only when saving the existing splitter tree. The
hook requires a recognized direct child of the pane shell, rejects nested/one-shot registrations,
and clears stale identity when a top-level switch cannot be represented. The shell prompt clears
the association after voluntary harness exit. UI teardown removes the runtime directory.

Restore passes validated data through the terminal's existing environment override interface.
Shell startup consumes it once, after user configuration, then invokes a fixed executable with
quoted arguments. Codex resumes with `--no-daemon`, retaining the local process-ancestry scope;
no hook-trust or permission bypass is added to product commands. There is no terminal-output
parser extension, periodic process scan, IPC service or renderer-path work for this feature.

Validation on macOS arm64:

- Required script release builds of eterm and the unit-test target passed. The rebuilt native
  terminal suite passed all **223 tests**, with hidden test windows.
- Python recovery tests passed, including real fish/Bash/zsh subprocesses, exact argv, UUIDv7,
  cwd quoting, one-shot consumption, custom harness homes, prompt clearing, nested ownership,
  session replacement and rejection of unsupported targets. Existing shell-integration tests
  passed; two PowerShell checks were skipped because PowerShell was unavailable.
- A hidden native inspector check restored two split fish panes in the same cwd to distinct
  exact IDs through local stand-in executables, and confirmed normal last-tab teardown. This
  verifies eterm's launch path, not a real conversation's reconstructed model context.
- A focused native diagnostic exercised capture/save-descriptor round-trip, malformed metadata,
  unsupported harnesses, UUIDv7, invalid owner PID, resume environment generation and cleanup.
- Real Claude 2.1.311 SessionStart capture succeeded using the user's existing authentication,
  without submitting a task. Real Codex 0.167.0 capture succeeded on its first turn, using the
  existing login environment with a refused localhost provider, without contacting a model
  service. The disposable Codex conversation was archived afterwards.

The initial isolated Claude startup check inadvertently advanced into authentication and caused
login prompts. That test approach was stopped; automatic review rejected a further isolated
retry. Subsequent live checks used the existing account after the user explicitly authorized it.
Future harness checks should detect authentication/onboarding screens and stop, not advance them.

Allocation/layout review: on macOS arm64 `TerminalConfig` remains **176 bytes**, alignment 8, and
its following enum still starts at offset 164; the new boolean uses existing padding. The recovery
owner is **72 bytes** (its map). Map entries, environment strings and hook directories are allocated
only on opted-in terminal creation. Directory strings are moved into the map; close callbacks
capture only the owner and terminal pointers. JSON and path buffers exist during save/restore;
there is no per-frame allocation or async UI ownership handoff. The Python helper's process table
and ancestor inspection run only on session events in the hook process.

Subsequent user-assisted Codex verification completed the real capture/save/reopen cycle in fish:
the hook recorded an exact session ID, the saved pane contained that same ID after the user closed
eterm, and the user confirmed Codex returned on reopening. No code or configuration changes were
made during this successful check. The prior failure's exact cause remains unproven; a session
that had not emitted its trusted hook is a plausible explanation. Codex 0.167.0 capture was observed
at its first turn, rather than merely on displaying the initial interface.

Linux behavior was not runtime-verified in the initial prototype; see the Linux follow-up below.
Automated full real conversation recovery after closure, custom launch flags/profiles, shared
servers, remote sessions, Windows and crash recovery remain outside the validation completed here.
This remains a convenience prototype, not a universal recovery promise.

## Linux Claude capture follow-up (2026-11-10)

The reported Linux failure was missing Claude hook configuration. The running native Claude
2.1.322 process inherited `ETERM_HARNESS_DIRECTORY`, `ETERM_HARNESS_BINARY` and the pane shell PID,
but its active settings contained no `hooks.SessionStart` entry and its private directory contained
no `active.json`. Codex's daemon relay does not require that hook, so working Codex recovery did
not establish that Claude was configured.

A fresh native Claude TUI, using the existing login and a temporary settings file with a
SessionStart hook, successfully registered its assigned UUID through the existing eterm helper.
The recorded owner PID matched the real Claude process. The hook payload supplied the expected
cwd and transcript path, and the helper returned silently. No prompt was submitted, authentication
or trust dialog accepted, model call made, or existing conversation opened. The five existing
Python recovery checks also passed on Linux, covering npm launchers, one-shot rejection, foreign
hooks, descriptor validation, exact resume argv and shell prompt cleanup.

This confirmed native Linux hook capture worked, but the manual setup requirement left eterm's
recovery setting incomplete. New opted-in panes now get a private `.claude-plugin/plugin.json`
inside the existing disposable hook directory. Its inline SessionStart hook invokes eterm's helper
with a fixed executable and an argument array, avoiding shell quoting on POSIX and Windows.

The pane appends its plugin directory to `CLAUDE_CODE_PLUGIN_DIRS`, supported since Claude
2.1.280. This leaves user settings, hooks, plugins, aliases and CLI flags intact, and also reaches
`claude --resume` during restoration. No account configuration is edited and no `claude` function
is installed. Claude's hook/plugin restrictions remain in effect. `resetEnvironment()` removes
only the inherited eterm plugin before creating a nested terminal; the separate
`ETERM_HARNESS_CLAUDE_PLUGIN` marker also survives Codex's relay clearing its capture directory.
Pane teardown removes the plugin along with the existing runtime record.

Before implementation, a real Claude 2.1.322 TUI loaded a temporary environment-supplied plugin
from a path containing spaces, an apostrophe, a dollar sign and Unicode. Its exec-form hook
captured the exact assigned ID while a separate SessionStart hook from explicit `--settings`
also ran. No model prompt was submitted. Full durable conversation contents after window
close/reopen remain outside this no-prompt capture check.

Validation of the implementation on Linux:

- The regenerated SDL3 release Ninja build of eterm passed.
- Real Claude 2.1.322 launched inside rebuilt headless eterm captured its assigned UUID with no
  manually installed hook. Normal Claude/shell exit closed the pane and removed the plugin.
- Headless pane checks verified opt-in/opt-out, preservation of unrelated plugin paths, removal
  of a parent pane's plugin in a nested terminal, and isolation when inherited relay state had
  an empty capture directory. Enabled panes removed their plugin directories on normal exit.
- Eight existing recovery/prompt/shell integration checks passed. Three PowerShell checks were
  skipped because PowerShell was unavailable. macOS and Windows runtime behavior was not retested.

Allocation/layout review: no instance members or object layouts changed. Plugin JSON, its serialized
manifest, path buffers and environment values are allocated only during opted-in pane creation.
The combined plugin list is moved into the environment map. Inherited lists are split/rebuilt only
when launching eterm from a pane that already carries its plugin. There is no per-frame work,
process polling, async capture or new long-lived allocation outside the existing pane environment
and private directory lifetime. Existing FileSystem and String operations provide the mechanism;
Claude-specific plugin policy stays in the application recovery owner.

[RexAgentRestore's installer](https://github.com/pattont/RexAgentRestore/blob/6668bde8be2bbb50fb6352f0dc4630517d718986/bin/rex-agent-restore)
also adds a Claude `SessionStart` hook to the agent's settings. That reference supports checking
per-agent hook installation separately from the terminal's recovery setting, but eterm's automatic
integration uses Claude's session-scoped plugin mechanism instead of editing those settings.

## Shared Codex daemon IPC follow-up (2026-11-07)

Daemon support should be investigated through its client protocol rather than extending the
hook's process-ancestry assumption. The installed Codex 0.167.0 exposes `app-server proxy --sock`,
daemon management commands, and locally generated JSON Schema for its app-server protocol.

Verified against the already-running local daemon, without creating/resuming a thread or reading
conversation contents: its control socket accepts an HTTP WebSocket Upgrade over a Unix socket;
`initialize` / `initialized` succeeds, followed by a successful `thread/loaded/list` request.
The `proxy` command forwards transport bytes: writing JSONL to it without the WebSocket handshake
did not establish an app-server connection in this check. No daemon restart, login, or model call
was needed.

The official [app-server protocol](https://learn.chatgpt.com/docs/app-server) documents thread
start/resume/fork, loaded-thread listing, notifications and Unix WebSocket transport. The installed
experimental schema was also inspected. It does not expose an evident local CLI process-ID to
selected-thread mapping. `remoteControl/client/list` concerns remote-control clients, not a general
local terminal selection registry. A thread's `originator` describes creation, not its current
client. Enumerating threads or matching cwd/recency cannot disambiguate two panes in the same cwd.

The promising next prototype is a separate local IPC relay for each opted-in pane:

1. Run the existing Codex TUI against that relay using `--remote unix://<pane-socket>`.
2. Forward its protocol connection to the existing daemon; leave authentication, approvals,
   generation and execution with Codex.
3. Match request IDs for `thread/start`, `thread/resume` and `thread/fork` to their responses,
   retaining only the returned thread ID and required recovery metadata for that pane.
4. Restore with the exact thread ID through the daemon. Verify session switching, multiple panes,
   errors, transport loss and ownership before replacing the current no-daemon prototype.

Record `thread.id`, not the newer `thread.sessionId` field: the latter can be shared by a root and
its forked session-tree members. Do not subscribe to or resume every thread just to observe state.
A relay must also avoid logging conversation payloads or changing bidirectional approval requests.
This would preserve the user's daemon and the normal Codex TUI, while establishing pane identity
at a connection the integration actually owns. The implementation follow-up below now prototypes
this design alongside the original standalone path.

### Daemon relay prototype implementation

`codex-session-relay.py` owns one invocation's private Unix socket, one forwarding thread and the
Codex TUI child. The opted-in fish/Bash/zsh startup integration installs a `codex` function unless
the user already defines one. Normal interactive invocations use `--remote` through that relay;
`--no-daemon`, explicit remote transports and recognized one-shot subcommands retain their original
invocation. There is no global PATH change or daemon restart. A missing default daemon can be
started through Codex's own management command.

The relay forwards raw handshake and WebSocket bytes in both directions, without changing approvals
or emitting model context. Its bounded observer matches start/resume/fork responses to requests
from that client. It records `thread.id`, cwd, transcript location, daemon socket and `mode=daemon`.
Selection in progress clears the old saved association; a failed selection restores it. Unsupported
or ephemeral selections clear it. This avoids choosing another pane by cwd or timestamp. Metadata
writes share the hook's existing atomic writer and disappear with the pane's runtime directory.

Saved standalone records (including older records without a mode) still restore with `--no-daemon`.
Daemon records restore through a newly created relay and the same upstream socket. C++ state does
not grow: mode and socket exist only in save/restore JSON and launch environment strings. The new
helper runs outside the renderer/UI process; WebSocket observation is capped at 8 MiB per message,
with oversized messages forwarded without capture. Large history responses can therefore leave
recovery unavailable. No conversation contents are logged, though frames necessarily pass through
the relay's transient memory.

Validation completed on macOS:

- Required script release build of eterm passed. Native descriptor diagnostics passed after the
  mode/socket additions. All seven relay/recovery tests passed, including fish/Bash/zsh restore
  argv for both modes, no-daemon/explicit-remote/one-shot bypass, failed and ephemeral selections,
  masked/fragmented frames, and byte-preserving approvals and ping traffic.
- A real isolated Codex 0.167.0 app-server, configured only with a refused localhost provider,
  was reached through the fish function and relay. Initial capture and exact-ID resume both
  registered the same daemon thread. Ctrl+C left the wrapper alive. No paid model request or
  authentication flow was used; the user's shared daemon was not restarted or reconfigured.
- The temporary validation server required forced termination after its graceful-stop deadline;
  cleanup was restricted to that test process. The production relay never stops the upstream
  daemon, only its own TUI child and local socket.

Still experimental: Linux runtime behavior, large response capture, unusual custom launch flags,
and the complete native-window daemon close/reopen cycle need broader checks. Daemon-side provider
configuration must exist on the server; defining a new provider solely in remote client flags was
rejected by the installed server during validation. Existing custom shell wrappers and explicit
`command codex` bypass automatic relay capture. This implementation intentionally retains the
standalone hook path rather than claiming universal client compatibility.

### Relay handshake fix

A reported `timed out upgrading remote app server` exposed a concrete accept-lifecycle flaw:
the first relay worker accepted only one connection. A connect-and-close probe could consume it,
leaving a listening socket with no worker for the real upgrade. The relay now accepts subsequent
connections and reconnects, with a maximum of eight concurrent workers and a separate request-ID
map per connection. Transport failures report only the failing stage and exception type, never
protocol payloads. Closed liveness probes remain quiet.

Regression checks cover a closed probe before a real upgrade, two concurrent protocol connections,
and repeated request IDs without cross-connection association. A probe followed by a read-only
handshake and loaded-thread query also passed through the fixed relay to the user's running daemon.
Real Codex capture and exact-ID resume passed again against an isolated localhost-provider server.
The user confirmed that retrying after this fix connects successfully. Other upgrade failures
still need their own diagnosis; not every possible timeout has the same cause.

### FreeBSD enablement and namespace cleanup

FreeBSD now enables the same recovery preparation and capture paths as macOS/Linux, for both
standalone hooks and the Unix-socket daemon relay. The C++ platform guard uses the existing `EE_PLATFORM_BSD` family.
The Python recovery tests also run on the BSD targets (`sys.platform` can include a release suffix).

The process snapshot query uses separate headerless `-o` fields and selects `ucomm` on FreeBSD.
FreeBSD's [ps(1)](https://man.freebsd.org/cgi/man.cgi?manpath=FreeBSD+14.0-RELEASE&query=ps&sektion=1)
documents that `comm` aliases command/arguments and that text after `=` can consume commas; using
the previous combined format could therefore break ownership detection. A focused regression
checks this command and ownership parsing against FreeBSD-shaped process data. No new platform
backend, allocation path or per-frame work is introduced.

Recovery declarations/definitions now use `UITerminal` and `UnorderedMap` with scoped namespace
imports, following project namespace style while retaining the terminal forward declaration.
Native FreeBSD compilation and execution were not available on the macOS development host.

### Windows recovery prototype and opt-in boundary

The common metadata path now accepts native Windows drive and UNC paths, creates an exclusive
UUID-named directory in the user's temporary directory, and uses OpenProcess/WaitForSingleObject
for a nonblocking owner-liveness check. POSIX preparation/capture also permits Haiku so a future
native harness does not require restructuring this layer; no Haiku harness availability or runtime
support is claimed.

PowerShell integration sources the optional recovery script after profiles and clears the active
association on the next prompt. It consumes resume requests once, uses literal paths and fixed argv,
preserves existing aliases/functions, and restores temporary harness-home overrides. Python's Windows
launcher is resolved to its interpreter so the relay remains a direct child of the pane shell.
Explicit standalone Codex launches bypass the helper and retain hook ownership. Windows hook ancestry
uses one bounded Win32_Process snapshot per event, recognizes native harnesses and cmd/node launchers,
and rejects nested and noninteractive harnesses. No execution-policy or account settings are changed.

Windows daemon mode uses a random bearer-token-protected loopback WebSocket endpoint. The native
`codex.exe app-server proxy --sock` process opens the existing daemon's control socket and carries
the unchanged WebSocket stream over stdin/stdout. This avoids assuming Python AF_UNIX support on
Windows. It requires native codex.exe on PATH; npm-only shims without an accessible native executable
are not resolved automatically. The two pipe directions run separately because Windows select does
not support anonymous pipes. Session selection is synchronized between those directions. Shutdown
closes only the private proxy/TUI; the upstream daemon remains running.

Validation on the macOS host: release eterm build passed; the Windows recovery C++ translation unit
cross-compiled to an x86_64 Windows object with Zig; 13 relay/recovery tests and four existing shell
integration tests passed. Portable PowerShell 7.5.4 exercised literal paths with spaces, apostrophes,
Unicode and brackets, Claude and both Codex resume modes, the Python launcher, request consumption,
standalone wrapper bypass and disabled recovery. Native-proxy fixtures checked byte forwarding,
bearer-token rejection and shutdown while a pipe read was blocked. Native Windows/PowerShell 5.1,
real Windows harness authentication and full window-close/reopen remain unverified.

Allocation audit: no recovery class members or UI-loop work were added. Windows preparation adds
one temporary UUID/path conversion and owned launch-environment strings per opted-in pane; owner
process handles close immediately. Process snapshots, Python buffers and proxy threads are outside
the renderer. Existing registration ownership and close/destruction cleanup remain in
HarnessSessionRecovery. Extra relay IPC, copies and helper/proxy memory have not been benchmarked.

Recovery disabled at pane creation skips prepare(), so eterm injects no recovery environment,
functions or relay. Existing open panes keep already-loaded wrappers until reopened. Previously
installed hooks remain user configuration and return without recovery work outside opted-in panes;
ordinary working-directory shell integration remains independent.

## Native helper refactor (supersedes the Python implementation above)

The native CLI dispatches `--harness-session` before constructing App, loading preferences or
initializing the UI. HarnessSessionRecovery owns bounded hook parsing, shared descriptor validation,
pane ownership and fixed-argv resume policy. CodexSessionRelay owns one invocation's authorization,
HTTP upgrade observation and thread-selection policy. The Python helpers are deleted. Thin
fish/Bash/zsh/PowerShell adapters validate attachment, preserve custom wrappers, invoke native
commands and clear metadata at the next prompt. No helper forks on every prompt. A bounded JSON
environment value carries the resume request and is consumed before harness execution.

The review's shared mechanisms moved into eepp with explicit approval:

- Process::InheritStdio borrows terminal handles and rejects pipe-only options. Process::runForeground
  owns signal forwarding, a bounded grace period, force-kill fallback and handler restoration.
  Process::requestStop requests graceful termination separately. Process data members are unchanged.
  subprocess.h retains its own indentation, and interrupted liveness checks retry waitpid.
- FileSystem::fileWriteAtomic exclusively creates a sibling, checks write/close, preserves existing
  POSIX permissions and replaces the destination, including on Windows. IniFile and CookieManager
  share it. IniFile retains symlink resolution and requests a flush; encoders and VFS retain their
  streaming/transaction semantics. FileSystem::createTempDirectory owns exclusive private-directory
  creation. Windows uses an owner-only DACL; POSIX uses 0700. isRelativePath recognizes UNC paths and
  preserves its existing drive-letter classification, including C: and C:relative.
- LocalSocket and LocalListener provide native local streams separately from TCP/TLS. Socket owns
  shared raw-stream I/O and timed connection behavior, including EINTR handling. POSIX IPC now uses
  these streams rather than another AF_UNIX connect/receive stack; its Windows named-pipe backend
  remains. A shutdown request interrupts an active IPC client, and an owned pipe wakes the worker
  independently of the socket pathname. Nonblocking accept stops on fatal errors instead of spinning.
  A listener removes its endpoint only when the device/inode still matches the bound path.
- StreamRelay owns its endpoints, bounded byte queues and readiness/backpressure policy, without a
  worker thread. eterm supplies its worker and protocol filter. It uses 64-KiB reads and bounds filtered
  output to 128 KiB per direction so a retained HTTP header can precede a chunk. SocketSelector supports
  read and write interests. There is no application-local socket platform code.
- String::constantTimeEquals compares equal-length byte strings without early exit; length is public.
  Sys::getRandomBytes supplies relay tokens, WebSocket keys/masks and inspector tokens. HTTP field
  validation is shared through Http::Response::parseFields.

WebSocket owns a unique_ptr<TcpSocket>, including SSLSocket for TLS. There is no borrowed transport
overload. It supplies client/server handshakes, synchronous message I/O and an independent decoder
for stream observers. It validates masks, fragmentation, UTF-8, close/control frames and bounds.
A partial frame send closes the stream permanently. Compression and subprotocols are not negotiated.
The relay accepts a valid 101 status line without a reason phrase.

Recovery activation now lets ShellIntegration resolve the empty default shell rather than checking
an unresolved program name. New panes scrub inherited recovery variables individually, including
shell PID and the relay token. Windows packages build and ship the console-subsystem eterm-session.exe
beside the GUI binary; a missing helper or failed private-directory creation logs a diagnostic.
Bash prompt hooks share one helper, use arrays only on 5.1+, preserve user commands/status and clear
metadata under noclobber. Settings text includes PowerShell and distinguishes standalone hooks.

Codex argument classification is shared by hooks and the relay. It parses known global options before
recognizing interactive resume/fork. Unfamiliar commands/options bypass recovery; initial command-line
prompts can use an explicit --. Default daemon availability is tested by connecting, not checking for
a socket file; refusal triggers one start attempt and a fresh probe. Custom endpoints stay with their
owner. Confirmed selection state is pane-wide, while pending request IDs remain connection-local.
Generations keep stale responses from undoing newer selections. Server requests do not consume client
response IDs. Failed switches restore confirmed metadata; ephemeral selections remain unsaved.

Sys::getProcessInfo uses bounded native identity/parent/optional argument inspection. Linux/Android
use procfs, macOS and FreeBSD use native process APIs, Windows uses snapshots/SDK process parameters,
and Haiku uses team/image APIs. Haiku's shortened display argument summary is not treated as exact
argv. iOS reports permitted fields without sandbox bypasses. Emscripten reports unavailable native
facilities. Recovery remains a desktop, best-effort prototype.

macOS helpers use @rpath library IDs, @loader_path and the standard framework search directory, so
protected hook shells do not need DYLD variables or a specific working directory. Standard project
build and packaging scripts remain the entry points. Runtime helpers do not depend on Python.

Ownership/allocation audit: each connection is stable under a unique_ptr in a bounded eight-slot
pool; the sole worker joins before connections or selection state are destroyed. StreamRelay owns
its endpoints and queues; one uninitialized 64-KiB stack buffer serves both directions per poll,
and only received bytes enter the retained strings. Blocking-mode syscalls occur only on transitions.
Protocol captures and JSON parsing stop when no selection is pending. Decoder callbacks are installed
once with small pointer/reference captures. The pane owns one last-confirmed descriptor; pending selections retain only IDs and generations,
without copying JSON snapshots. Interrupted or oversized selections restore that confirmed descriptor
and log the limitation. Oversized replies continue through the relay and cannot leave requests pending. Command/environment strings own their data across subprocess creation. No protocol
or process polling occurs in the GUI render loop.


Current macOS layout measurements: Sys::ProcessInfo 56 bytes, Process 192, Socket 24, TcpSocket 72,
LocalSocket 24, LocalListener 64, WebSocket::Decoder 176, WebSocket 240, and
StreamRelay<TcpSocket,LocalSocket> 200. Existing Process and Socket layouts remain unchanged.
Compiler record layouts also confirm Direction 208 bytes (previously 216), Selection 144,
Connection 784, and Relay 248. Direction groups both flags at the end; Relay has one tail padding
region. LocalListener retains device/inode identity to avoid unlinking a replacement endpoint.
Upstream connections are nonblocking and share the relay worker's selector and five-second deadline;
one stalled connection cannot block other connections. The worker retains one selector across polls.

Validation of the native refactor and review fixes:

- macOS release eterm and unit-test builds succeeded through the required project scripts.
- All 111 focused framework tests passed, covering process lifecycle/graceful stop, native identity
  and randomness, atomic files and INI/cookie persistence, IPC, local streams, selector interruption,
  WebSocket framing/partial sends, bidirectional backpressure, HTTP and TLS.
- Nine native recovery/relay tests passed against both the raw executable and the rebuilt app bundle.
  These exercise exact standalone and daemon resume IDs, invalid records, bypass arguments with
  global options, stale daemon sockets, failed/ephemeral selections, request-ID namespaces,
  fragmentation, multiple connections, approvals and PowerShell adapter isolation.
- Four existing shell integration tests and the new prompt-hook regression passed. Real local
  Bash 5.0 and 5.1 Docker images also preserved user prompt commands, status, OSC 7 and noclobber.
  Native Bash 3.2, zsh, fish and portable PowerShell were exercised without accounts or model calls.
- Hidden native windows with isolated homes/profiles verified empty-shell resolution to fish,
  enabled attachment, disabled non-injection and inherited relay-token scrubbing. The bundle
  contains the native shell adapters and no Python recovery scripts or bytecode.
- A native fake Codex executable verified positive hook ancestry and rejection of exec after global
  options. It made no model request. Thirteen changed translation units cross-compiled for Windows,
  including the console helper; Windows end-to-end recovery still needs native runtime validation.

The two existing user-level eterm hooks were migrated from Python commands to native helper commands,
with backups and other settings preserved. Changed Codex hooks need the user's normal /hooks review.

The final Linux arm64 release Ninja aggregate also passed in the isolated Docker image. The same
111 focused framework tests all passed there. Linux native recovery tests passed (eight active,
one PowerShell-only skip), as did the Bash prompt regression and existing shell startup/directory
checks (PowerShell checks skipped because it is absent in that image). The final C/C++ snapshot
matched the workspace before the temporary container was removed. Both MinGW and PowerShell
packaging fixtures included eterm-session.exe and rejected a missing helper. The rebuilt macOS
bundle passed signature verification, native recovery tests and hidden default-fish/opt-out checks.

### Native Linux, Windows and Haiku validation

The same source snapshot was subsequently built on native x86_64 systems, in separate directories
that preserved the existing checkouts:

| Platform | Native build | Framework checks | Recovery checks |
| --- | --- | --- | --- |
| Linux | Full release Ninja aggregate with SDL3 and mold | 111 passed with SSL_CERT_DIR unset for the test process | Eight passed, one PowerShell-only skip; prompt regression passed |
| Windows 10, GCC 15.2 | Release eterm, eterm-session.exe and unit tests with MinGW | All 99 applicable tests passed, including TLS | Six native checks passed; existing adapter/CLI checks passed with POSIX fixture skips |
| Haiku hrev60337, GCC 13.3 | Release eterm and unit tests with Premake 4 | 109 passed; TLS filter timed out, also with the existing binary | Eight passed, one PowerShell-only skip; prompt regression passed |

Windows checks verified that the helper is a console-subsystem executable, eterm remains a GUI
executable, and PowerShell observes the helper's exit status. Standalone Codex and Claude resumed
exactly once with the exact ID, consumed the request, preserved Unicode working directories and
cleared metadata at the prompt. Native daemon tests covered resume, stale default sockets, bypass
arguments, failed/ephemeral selections, multiple connections, fragmented frames and approvals.
A compiled console-only fake harness delegated to the mock protocol client. The installed Windows
Python omitted AF_UNIX address support, so the mock server used Winsock directly through ctypes.
POSIX SIGINT injection was excluded from this pipe-only Windows SSH check; actual harness accounts
and model calls were not used.

Native validation found two test/build portability issues: the Windows unit target needed explicit
ole32/uuid links for its direct OLE tests, and saved socket endpoints must be compared as filesystem
paths rather than separator-sensitive strings. Both were fixed. The portable process suite is now
also enabled on Haiku; all 16 process tests passed there. Windows Make used the installed POSIX shell
to avoid cmd.exe's long-command truncation; this changed only the build invocation.

The initial Linux TLS failures came from the SSH environment: SSL_CERT_DIR pointed at the system
CA directory and overrode the test's explicit CertificatesPath. Both TLS cases passed when that
variable was removed for the test process. The fixture now temporarily removes that override,
then restores it; production keeps the user's environment precedence. Existing binaries were not a
reliable baseline for declaring these failures acceptable.
Haiku's TLS timeout remains under investigation. No system TLS configuration was changed.

### Second review corrections

IPC shutdown now wakes an owned pipe rather than reconnecting through its pathname. The listener
accepts only after readiness, retries interruptions in LocalListener, and stops on persistent accept
errors. Regression tests remove/replace the endpoint, exhaust descriptors, close, and listen again.
LocalListener checks the endpoint identity before removing it. Socket connect distinguishes Linux
AF_UNIX queue exhaustion from a pending connection and verifies the peer before reporting completion.

The relay retains one confirmed descriptor per pane. Oversized observations, interrupted selections
and malformed selection results restore that descriptor with a diagnostic; pending IDs are cleared,
and subsequent selections remain observable. The original oversized response is still forwarded.
Upstream connections are nonblocking. npm hook ownership recognizes the known package entry points
through node, nodejs, and versioned Node executables on POSIX as well as Windows. Modern Node
can label its Linux main thread MainThread; detection still verifies its executable and entry point.

The user approved conventional POSIX signal exit codes (128 + signal), last-confirmed recovery after
interrupted/oversized selections, and targeted Windows CTRL_BREAK forwarding for Ctrl-C without a
forced-shutdown timer. The ambiguous signal-taking Process helper was removed. SSL_CERT_DIR retains
its production precedence; only the loopback TLS fixture isolates and restores that environment.
Drive-letter classification retains existing behavior. Mach-O builds no longer add ELF $ORIGIN paths.
Socket source files retain their original CRLF endings; third-party subprocess edits retain spaces.

Full macOS release suite: 2,434 passed, four skipped, zero failures. Full native X11/Xvfb Linux release
suite: 2,441 passed, six skipped, zero failures, including TLS with the original SSH environment.
The headless Linux probe had four rendering failures; native X11/Xvfb resolved them. The macOS raw
binary and rebuilt bundle each passed all ten recovery checks. Final Windows validation follows below.
Haiku rebuilt successfully; additional runtime validation stopped when the user closed that VM to
prioritize Windows memory. Its previously observed TLS timeout is still unexplained, not accepted
as a baseline or claimed to pass.

Final Windows release validation used SDL3 and Mesa 23.3.5/llvmpipe, matching CI's backend and
renderer: 2,329 passed, 17 skipped, zero failures (2,346 registered tests). All six native recovery
checks and the hidden-console Ctrl-C regression passed against the fresh framework. The standard
CLI/adapter checks passed as well (five passed, five platform fixture skips).

The Windows SDK needed to be added to PATH/LIB so Premake discovered SDL3 instead of silently
falling back to an installed SDL2. Locked DLLs and old test objects survived `make clean`; moving
those artifacts aside and rebuilding their object directories was necessary. Imports were checked
explicitly before the final run. No production backend-selection behavior was changed.

One portable terminal test assumed that the first read slice always completed 1,024 reads. Its
implementation also has a four-millisecond time limit, reached earlier on this Windows VM. The
assertion now checks positive progress within the read limit; the exact complete-output and redraw
assertions remain. Terminal behavior was unchanged. On macOS, relinking only the test target through
the non-framework script mixed Homebrew SDL with the framework linked by eepp; the framework script
restored consistent linkage before final validation.

Final full-suite results after the timing-test correction and coherent SDL linkage:

| Platform | Passed | Skipped | Failed |
| --- | ---: | ---: | ---: |
| macOS, SDL2 framework | 2,434 | 4 | 0 |
| Linux, SDL3, native X11/Xvfb | 2,441 | 6 | 0 |
| Windows, SDL3, Mesa/llvmpipe | 2,329 | 17 | 0 |

The final Linux recovery suite passed nine checks with one PowerShell-only skip; macOS passed all
ten against both raw and packaged executables. Windows passed six native recovery checks, five
applicable CLI/adapter checks and the isolated hidden-console Ctrl-C check. Harness authentication,
real conversations and model calls were not used during these checks.

### Third review corrections

These supersede the corresponding statements in the second review corrections above.

- POSIX IPC no longer stops on accept errors other than an unusable listener (`EBADF`, `EINVAL`,
  `ENOTSOCK`). Descriptor or memory exhaustion leaves the connection queued, so the worker backs
  off for 100 ms, interruptible by `close()`, and retries. Stopping left the endpoint bound and
  connectable while every message was lost, and a new instance saw it as already owned. The
  exhaustion regression test now expects delivery after descriptors are released.
- The Windows IPC worker disconnects a pipe instance whose `ConnectNamedPipe` failed before
  retrying it. `ERROR_NO_DATA` (a client connected and closed first) previously spun forever.
- An oversized observation, an interrupted selection or a malformed selection result now clears
  the saved Codex session instead of restoring the last confirmed one. The daemon may already have
  switched threads, so restoring the old record could resume the wrong conversation, which the
  failure policy above forbids. Only an explicit error response restores the confirmed record.
- `Process::runForeground` no longer creates a Windows process group. That flag disables Ctrl-C in
  the child, which is why Ctrl-C had been forwarded as CTRL_BREAK and ended Node and Rust harnesses
  instead of interrupting them. The child now shares the console like any directly run command and
  receives console events itself; the runner only survives them and applies the grace period.
  Verified on native Windows 10 with a hidden-console fixture: the child receives
  `CTRL_C_EVENT` (not CTRL_BREAK), `eterm-session` keeps running 2.5 s later, and the child's exit
  status is propagated.
- An interrupted blocking `connect()` is no longer retried, which reported `EALREADY`/`EISCONN` as
  errors. It is awaited like a nonblocking connect.
- A Codex positional argument containing whitespace is treated as an initial prompt, so
  `codex "fix the build"` is recovered. Subcommand names contain no whitespace; single bare words
  still pass through unchanged.
- ecode's macOS release `-g1 -fvisibility=default` flags, removed by mistake with the rpath change,
  are restored.
- Wine has no AF_UNIX sockets (`socket()` fails with `WSAEAFNOSUPPORT`). `Sys::isRunningOnWine()`
  exposes the existing detection, and the LocalSocket stream test skips under Wine; it passes on
  native Windows.
- Harness process IDs use `ProcessID` instead of `int`, removing MinGW sign-compare warnings, and
  the saved `owner_pid` must be an unsigned integer.

### Fourth review corrections

- Selection state is tracked per pane rather than per connection:
  - The pane counts unresolved selections across all of its connections. The confirmed record is
    saved again only when none remain, so a newer error no longer restores it while an older
    selection could still switch threads.
  - An error never changes the record.
  - A success from an older selection drops the record, because the daemon may be on either
    thread. Dropping it clears the saved file at once, even while other selections are
    unresolved; only republishing a known record waits for them.
  - A connection that closes with a selection unresolved drops the record only if that
    selection was requested after the record was confirmed.
- Outgoing observation loss clears recovery even when no selection was captured:
  - An oversized request, or a 65th concurrent selection on one connection, counts as an unseen
    newer selection, so responses to requests sent before it cannot save their result.
  - If outgoing frames cannot be decoded at all, the connection is marked blind and no session
    is saved until it closes.
- A whitespace prompt no longer ends argument checking: later options still decide, so
  `codex "fix the build" --remote ...` and `codex "fix the build" --help` are passed through.
  A second bare word or `resume`/`fork` after the prompt also passes through.
- Known limitation: Windows does not send logoff or shutdown console events to processes that
  loaded `user32.dll`. `eterm-session` loads it through eepp, so its grace period is not guaranteed
  for those events.

### Final review corrections

- npm hook ownership resolves the launcher script path before matching: node reports the symlink
  npm puts on PATH, not the package entry point, so npm Codex run outside the shell wrapper was
  never recorded.
- npm Claude sets `process.title`, which overwrites its visible arguments (macOS keeps the `node`
  name, Linux takes `claude`). A node process whose arguments read `claude` followed only by empty
  strings is recognized as Claude, and Claude's exported `CLAUDE_CODE_ENTRYPOINT` (`cli` for the
  TUI, `sdk-cli` for `--print`) decides one-shot runs, falling back to arguments when unset.
  The variable was read from the Claude 2.1 binary; it has not been checked with a real npm
  Claude.
- Saved records carry the owner's start time, so a reused PID no longer keeps a stale record.
  On Linux, `Sys::getProcessCreationTime` derived that time from the current time minus a rounded
  uptime, so repeated reads of one process differed by a second; it now uses the boot time from
  `/proc/stat`, and parses `/proc/<pid>/stat` after the command name, which may contain spaces.
  Whole seconds suffice: a mismatch needs the owner to exit and its PID to be reused within the
  second it started.
- The relay removes its bearer token from the upgrade request before forwarding it to the daemon.
- When all 8 relay slots are used, a new connection replaces the oldest unauthenticated one, so
  idle local connections cannot lock the TUI out.
- Client messages are parsed only if they can name a `thread/*` method (plainly or through JSON
  escapes), so large prompts and pasted images no longer build a JSON document on the relay worker.
- `WebSocket::receive()` after a local `close()` reads until the peer's Close instead of returning
  at once, still answering pings until then (only data frames are forbidden after our Close). Once
  both Close frames are exchanged it disconnects the transport, so the peer is not left waiting
  for TCP to close.
- Sockets are created and accepted close-on-exec atomically on Linux, Android and the BSDs
  (`SOCK_CLOEXEC`, `accept4`); macOS and Haiku keep the `fcntl` fallback.
- Known limitation: the prompt hook clears the saved record whenever the shell prompt returns,
  including after a harness is suspended with Ctrl-Z. After `fg`, recovery is restored only by the
  next thread selection (Codex relay) or SessionStart (hooks).

### Fish startup prompt duplication with vertical tabs

Restoring a terminal with vertical tabs can start fish with the full layout width before the tab
bar reduces the terminal's width. The emulator resized its grid immediately but deferred the PTY's
size notification for 100 ms. Fish could therefore draw its first right prompt using the old width
into the narrower grid. The right prompt wrapped onto another row; the later SIGWINCH repaint drew
a second left prompt on that row, leaving the first one visible.

An isolated copy of the local fish prompt functions reproduced this with harness recovery both
on and off. A PTY trace showed fish reading 182 columns, drawing its right prompt at column 175
into an already 153-column grid, then receiving the 153-column size notification. The startup
injection in `ShellIntegration` and `harness-session.fish` did not generate an extra command.

An initial experiment removed the PTY-only debounce and updated the PTY and grid together. It
eliminated the reproduced duplicate prompt, but also removed coalescing of shell/application resize
notifications. That implementation and its test changes were backed out after reviewing the original
rationale: Codex and Claude can perform expensive relayout work on each notification. The existing
100 ms debounce and its regression coverage are retained.

The fix changes restoration order instead. `UITabWidgetSplitter::prepareRestore()` builds the
complete split tree, applies saved proportions, and reserves tab-bar space for registered saved
pages. It returns a move-only `Restoration` handle without invoking page loaders or flushing the
scene. eterm retains that handle and registers one cancellable `UISceneNode::onLayoutReady()`
subscription. The scene dispatches readiness work after its normal layout passes only when waiters
exist and style/layout queues are clear. A callback that dirties geometry defers remaining waiters;
callbacks registered during dispatch wait for a later normal update. Subscriptions are removed before
invocation, and cancellation is rechecked against the listener registry even during snapshot-based
dispatch. There is no permanent eterm update hook, forced scene pass, timer, or fixed frame count.

Once ready, `loadWidgets()` invokes the destination-aware factories in the owning scene's context.
Each shell starts with its tab widget's content size rather than `mainLayout` dimensions. Page
creation happens outside scheduled-widget traversal, so subscribing the new terminal is safe.
The registered-loader `fromJSON()` API remains synchronous and does not flush scene layout.
The normal 100 ms PTY resize debounce is unchanged.

The pending state owns the saved JSON by move across ticks. Its pane descriptors borrow that JSON;
close listeners invalidate widget pointers if panes disappear. Completion consumes the state once,
and cancellation or loader exceptions restore the original tab-bar policy and disconnect listeners.
The splitter must outlive the handle; eterm enforces that order with scope cleanup. Closing eterm
before readiness saves the owned original session instead of overwriting it with an empty layout.
A page that fails to load can still change the final tab count and therefore the bar's visibility.

Validation of the final implementation:

- Release eterm and unit-test builds succeeded through the configured Linux Ninja workflow.
- All 310 focused native tests passed: 34 tab-widget, nine splitter, 223 terminal, 27 scene, six
  invalidation and 11 event-connection cases. The 15 splitter cases also passed separately.
- New regressions use normal scene ticks for every tab position and nested split. A deliberately
  dirty layout takes three ticks; factories cannot run early and load only once. Other checks cover
  cancellation/moves, a destroyed pending pane, loader exceptions and restoring the caller's scene
  context after a factory executes under another ambient scene. The existing deferred-resize timer
  regression remains unchanged. Readiness-specific checks cover single firing, multi-tick layouts,
  safe scheduled-widget creation, cancellation during dispatch, new subscriptions during dispatch,
  scene destruction and throwing callbacks.
- Native recovery passed five checks; shell integration and prompt hooks passed three applicable
  checks, with two PowerShell checks skipped because PowerShell is absent.
- Offscreen fish restores with the user's datetime prompt showed one prompt per visible terminal,
  in a single pane and restored splits. The PTY trace starts fish at 153 columns and keeps that
  width through the first prompt instead of starting at 182 columns and then shrinking.
- Cross-platform runtime validation was not repeated for this fix.

Allocation/API audit: `UITabWidgetSplitter`, `UISceneNode` and `WidgetTypeCallback` retain their
496-, 4,360- and 64-byte Linux x86_64 layouts (compiler sizeof checks). The new move-only handle is
eight bytes. Its one pending state allocation is 240 bytes (compiler debug layout); pane and close
connection vectors each keep four entries inline and grow only for larger trees. JSON moves preserve
ownership without copying the document. Close callbacks capture two pointers and the shutdown guard
captures one pointer, fitting inline callback storage. The state and connections are released when
loading finishes or is cancelled. eterm keeps one readiness EventConnection and disconnects it
before splitter teardown; no restore hook remains in its render loop. The UI readiness facility
reuses the existing event registry and adds no Node or UISceneNode data members. Its callback wrapper
moves the user's callable into registry storage; the normal event snapshot copies it only when
settled callbacks are dispatched, never on pending dirty-layout ticks. Small wrapper allocations
at registration/dispatch follow the existing std::function event machinery. An empty event bucket
is not considered an active listener, so completed or cancelled waiters do not keep dispatching.

#### Resize debounce history and remaining validation

- `fe1149ffc` (2026-03-09, "Terminal Reflow WIP") introduced history reflow and the new grid
  resizing implementation, before the debounce existed.
- `a8384f183` (2026-03-12) introduced the 100 ms delay. Its message explicitly describes
  primary-screen reflow debouncing and better prompt readjustment. The code set `is_syncing`
  before grid restructuring, cleared the prompt tail, sent a prompt-refresh sequence, restarted
  the delay on each resize, and notified the PTY once the delay expired. Alternate-screen
  resizes bypassed the delay. The draw guard using `is_syncing` was commented out in that commit,
  so suppressing intermediate frames was an expressed intention rather than an active guard.
- `afb055405` (2026-03-15, "Do not change terminal scroll position on terminal resize") removed
  the forced prompt-refresh sequence and preserved scroll position, while retaining the delay.
- `c5c635092` (2026-08-31, "eterm: harden threaded presentation and terminal protocol handling")
  implemented real DEC mode 2026 synchronized presentation and removed the resize path's writes
  to that synchronization flag. The PTY debounce remained.
- `b14379ed3` (2026-11-03) explicitly retained the deferred-resize deadline when replacing worker
  polling with readiness waits, and added a timer regression for it.

The 100 ms interval is a practical coalescing window: the user confirmed that it avoids repeated,
expensive relayout in applications such as Codex and Claude. That behavior is preserved by fixing
the initial restore geometry instead of changing subsequent resize notifications.
