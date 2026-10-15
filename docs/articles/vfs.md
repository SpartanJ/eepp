# Virtual File System (eepp-vfs)

## Introduction

`eepp-vfs` is an optional static module that gives applications one asynchronous, URI-based API
for local files, packed resources and remote servers:

| Scheme | Provider | Notes |
| --- | --- | --- |
| `file://` | `LocalProvider` | Native filesystem (POSIX and Win32). |
| `res://` | `ResourceProvider` | Read-only view of every open `Pack` (`System::PackFileIndex`). |
| `sftp://` | `SFTPProvider` | Built by default (`--without-vfs-sftp` disables it), libssh2 based, honors `~/.ssh/config`. |
| `mem://` | `MemoryProvider` | In-memory filesystem, mainly for tests. |

The module builds on the [async core](async_core.md), generic primitives that live in eepp core and
are useful on their own:

| Header | Type |
| --- | --- |
| `<eepp/core/result.hpp>` | `Result<T, E>`, `Ok()`, `Err()` |
| `<eepp/core/task.hpp>` | `Task<T>`, `TaskSource<T>`, `makeReadyTask()` |
| `<eepp/core/cancellationtoken.hpp>` | `CancellationSource`, `CancellationToken`, `CancellationRegistration` |
| `<eepp/core/executor.hpp>` | `Executor`, `InlineExecutor` (`ThreadPool` is an `Executor` and has `submit()`) |
| `<eepp/scene/mainthreadexecutor.hpp>` | `MainThreadExecutor`: runs continuations on the UI thread |
| `<eepp/core/uri.hpp>` | `URI` (formerly `Network::URI`, which remains an alias) |

Link the module with `eepp_module_vfs_add()` in Premake and include `<eepp/vfs.hpp>`. The UI model
lives in `<eepp/vfs/ui/vfsdirectorymodel.hpp>` and is not part of the aggregate header.

## Tasks, results and threads

Every VFS operation returns `VFSTask<T>`, an alias of `Task<Result<T, VFSError>>`:

```cpp
auto& vfs = EE::VFS::instance();
MainThreadExecutor ui( sceneNode );

vfs.stat( URI( "sftp://makyo/home/black/notes.txt" ) )
	.thenOn( ui, []( VFSResult<FileInfo>&& info ) {
		if ( info )
			label->setText( info.value().name );
		else
			label->setText( info.error().toString() );
	} );
```

Rules:

- VFS methods never block and may be called from any thread.
- Tasks resolve on VFS threads. `then()` runs where the task resolves; `thenOn( executor, ... )`
  always goes through the executor. Touch UI or model state only from `thenOn( uiExecutor, ... )`.
- A `Task` has one consumer: continuations consume it (`std::move( task ).then( ... )`).
  A continuation returning another task is flattened.
- If a producer is destroyed without resolving, its task chain becomes `Abandoned` instead of
  hanging. That is an infrastructure bug (reported by `TaskDiagnostics`), never a domain outcome.
- Cancellation is cooperative: pass `CancellationToken`s; cancelled operations complete with
  `VFSErrorCode::Cancelled`.
- `Task::wait()`/`get()` block. They exist for worker-thread adapters (for example `IOStreamVFS`)
  and tests; never use them on the UI thread.

Errors are typed (`VFSErrorCode`): callers branch on codes such as `NotFound`,
`PermissionDenied`, `ChangedExternally` or `HostKeyRejected`, never on errno or libssh2 values.

## Files and directories

```cpp
// Incremental listing: batches with an explicit end-of-directory flag.
vfs.openDirectory( uri );    // VFSTask<DirectoryHandle>; then handle.read( 256 ) per batch

vfs.listAll( uri );          // convenience: the whole directory
vfs.readAll( uri );          // VFSTask<std::vector<Uint8>>
vfs.writeAll( uri, data );   // atomic by default, returns the new FileInfo
vfs.makeDirs( uri );
vfs.remove( uri, RemoveOptions{ true } );   // recursive
vfs.rename( from, to, RenameOptions{ true } );
vfs.canonicalize( uri );     // resolves symlinks/aliases; URIs are never canonicalized implicitly
```

`FileHandle` uses explicit offsets (`read( offset, span )`, `write( offset, span )`); `FileCursor`
adds a sequential position. `IOStreamVFS` exposes a VFS file as a blocking `System::IOStream` for
existing decoders running on worker threads.

`FileInfo` is a plain value. Unknown metadata stays unknown (`std::optional` accessors). It does
not store its own URI, because listings hold many entries and the caller always knows the URI it
listed: use `info.uriIn( directory )` for a listed entry (`VFSDirectoryModel::entryURI( row )` in
the model). `FileInfo::revision` is an opaque token used for conflict detection:

```cpp
WriteOptions options;
options.expectedRevision = lastKnown.revision;   // from a previous stat/writeAll
vfs.writeAll( uri, text, options );              // fails with ChangedExternally if modified
```

Atomic writes go to a sibling temporary file, are flushed and renamed over the destination; a
symlinked destination is written through to its target. Providers report what they can do through
`capabilities( uri )` (for example `AtomicReplace`, `Fsync`, `Permissions`).

### Copy and move

Recursive and cross-provider transfers are orchestrated by the VFS, not by providers:

```cpp
Transfer transfer = vfs.copy( URI( "sftp://makyo/srv/www" ), URI::fromFSPath( "/tmp/www" ) );
auto progress = transfer.onProgress( []( const TransferProgress& p ) { ... }, &uiExecutor );
transfer.completion().thenOn( uiExecutor, []( VFSResult<TransferResult>&& r ) { ... } );
transfer.cancel();
```

Moves use a native rename when both sides are on the same provider/authority and fall back to copy
plus removal of the entries that were completely copied; a source is never removed after a partial
transfer.

File copies are written to a temporary sibling before publication. `Fail` and `Skip` also apply
when a destination appears while the copy is running. The `Overwrite` policy requires
`AtomicReplace`; otherwise the transfer returns `Unsupported` and preserves the old contents.
Creating a file with `Fail` or `Skip` requires `Rename`. Moving a symlink through the copy fallback
is unsupported; native same-filesystem renames still move the link itself.

## Watching

```cpp
vfs.watch( uri, WatchOptions{ /*recursive*/ true } )
	.thenOn( ui, [this]( VFSResult<WatchHandle>&& watch ) {
		mWatch = std::move( watch ).value();
		mWatch.onEvents( ui, [this]( WatchEventBatch&& batch ) { ... } );
	} );
```

Events are relative to the watched root and arrive in coalesced batches through the executor you
choose. `RescanRequired` means continuity was lost (missed native events, reconnection, overflow):
re-list the directory. `WatchHandle::status()` reports `Active`, `Polling`, `Reconnecting` or
`Failed`, so a broken remote watch never looks healthy.

Backends:

- `file://`: efsw native watching; directories efsw reports as remote (NFS, SMB) use efsw's
  generic polling watcher.
- `sftp://`: `efsw-agent --stdio` runs on the server through an exec channel of the same SSH
  connection (one agent per connection, many watches). When the agent dies or the connection drops,
  watches report `Reconnecting`, re-register and emit `RescanRequired`. If the agent is not
  installed they fall back to polling over SFTP. SFTP itself never depends on the agent.
- Anything else: polling through the VFS (`PollingWatchBackend`).

To establish a consistent view, watch first, list second, then apply the events that arrived during
the listing; `VFSDirectoryModel` does exactly that.

## VFSDirectoryModel

A `UI::Models::Model` for one directory of any scheme:

```cpp
auto model = VFSDirectoryModel::New( uiExecutor );
tableView->setModel( model );
model->setURI( URI( "sftp://makyo/home/black" ) );
```

It enumerates incrementally (the first rows appear after the first batch), discards results of
previous navigations, stays live through watches (row inserts/deletes keep the selection), and
exposes `loadState()`, `error()` and `watchStatus()`.

Columns are Name, Size, Modified, Type, Permissions, Owner and Group; cells carry a per-column CSS
class (`vfs_size`, ...). Applications customize the text with `setTypeProvider()`,
`setTimeFormatter()` and `setItemCountFormatter()`. With `Config::countDirectoryItems` (or
`setCountDirectoryItems()`), directories are counted in the background and the Size column shows
"N items". `setNameFilter( pattern, mode, caseSensitive )` filters rows by name with unanchored
`PlainText`, `Glob` or `RegEx` patterns, like Dolphin's filter bar.

## SFTP

SFTP is built by default (disable it with `--without-vfs-sftp`, Premake 4 or 5; it is never built
for emscripten). This compiles the vendored libssh2
(`src/thirdparty/libssh2`) against eepp's mbedTLS, or OpenSSL when `--with-openssl` is used.

`sftp://host/path` resolves `host` through OpenSSH configuration exactly like `ssh host` would:
`~/.ssh/config` and the system `ssh_config`, `Host`/`Match` (all client criteria), `Include`,
first-value-wins, `HostName`, `Port`, `User`, `IdentityFile`, `IdentityAgent`, `IdentitiesOnly`,
`ProxyJump`, `ProxyCommand`, `UserKnownHostsFile`, `StrictHostKeyChecking`, `HostKeyAlias`,
algorithm lists, hostname canonicalization and the final re-parse pass. A user or port in the URI
overrides the configuration. Passwords are rejected in URIs. `SSHConfigEvaluator` is usable on its
own and is tested against `ssh -G`.

Ed25519 identity files and host keys and the `curve25519-sha256` key exchange are supported with
both crypto backends. mbedTLS does not implement them, so the vendored libssh2 gets them from
Monocypher (`src/thirdparty/monocypher`, `src/thirdparty/libssh2/src/mbedtls_ed25519.c`).

Host keys follow `StrictHostKeyChecking`. Unless `HostKeyAlgorithms` is configured, the host key
algorithms that known_hosts already has keys for are preferred, as OpenSSH does. Changed keys are always refused. For unknown keys with
`ask`, the provider asks an application `SSHHostKeyVerifier`; secrets (passwords, passphrases,
keyboard-interactive answers) come from an `SSHAuthenticationHandler`. Both return tasks, so the
SSH I/O thread never blocks while the user answers. Accepted keys are appended atomically to the
user's known_hosts file.

Authentication order: ssh-agent, identity files (asking for passphrases), keyboard-interactive,
password, filtered by `PreferredAuthentications` and the `*Authentication` options.

Connections are pooled per effective configuration and shared by every operation and watch. All
sessions are driven by one non-blocking I/O thread. `posix-rename@openssh.com` (atomic replace),
`fsync@openssh.com` and `statvfs@openssh.com` are detected per server.

### efsw-agent

`efsw-agent` is part of efsw (`src/agent/efsw-agent.cpp`, protocol in
`include/efsw/agent_protocol.hpp`). It has no eepp dependency and only reports changes. Install it
on the server's `PATH`, or set `SFTPProviderConfig::watchAgentCommand`. The eepp build produces
`bin/efsw-agent` for the build host.

### Known limitations

- FIDO security-key identity files (`ssh-keygen -t ed25519-sk` / `ecdsa-sk`) are not supported
  with either crypto backend: signing needs the hardware token (libfido2), which libssh2 leaves to
  the application. Such keys can be used through ssh-agent, which signs on the token's behalf
  (untested).
- `Match localnetwork` is not implemented on Windows.
- Jump hosts use their own `ssh_config` entries, but their own `ProxyJump`/`ProxyCommand` are
  ignored (the chain comes from the destination).
- Operations on one SSH connection are serialized; a long transfer delays metadata requests on the
  same connection.
- The efsw-agent is not deployed automatically.

## Resources (`res://`)

`System::PackFileIndex` (formerly `System::VirtualFileSystem`, still available as a deprecated
alias) indexes the files of every open `Pack`. `res:///fonts/a.ttf` maps to the pack path
`fonts/a.ttf`; directories are implicit. The provider is read-only and extracts files to memory
when they are opened.
