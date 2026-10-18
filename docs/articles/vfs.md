# Virtual File System (eepp-vfs)

## Introduction

`eepp-vfs` is an optional module that gives applications one asynchronous, URI-based API
for local files, packed resources and remote servers:

| Scheme | Provider | Notes |
| --- | --- | --- |
| `file://` | `LocalProvider` | Native filesystem (POSIX and Win32). |
| `zip://` | `ZipProvider` | Read-only browsing and streaming reads of individual local ZIP archives. |
| `archive://` | `ArchiveProvider` | Read-only local archives through optional runtime-loaded libarchive 3.x. |
| `res://` | `ResourceProvider` | Read-only view of every open `Pack` (`System::PackFileIndex`). |
| `sftp://` | `SFTPProvider` | Built by default (`--without-vfs-sftp` disables it), libssh2 based, honors `~/.ssh/config`. |
| `mem://` | `MemoryProvider` | In-memory filesystem, mainly for tests. |

The module builds on the [async core](async_core.md), generic primitives that live in eepp core and
are useful on their own:

| Header | Type |
| --- | --- |
| `<eepp/core/result.hpp>` | `Result<T, E>`, `Ok()`, `Err()` |
| `<eepp/core/task.hpp>` | `Task<T>`, `TaskResolver<T>`, `makeReadyTask()` |
| `<eepp/core/cancellationtoken.hpp>` | `CancellationSource`, `CancellationToken`, `CancellationRegistration` |
| `<eepp/core/executor.hpp>` | `Executor`, `InlineExecutor` (`ThreadPool` is an `Executor` and has `submit()`) |
| `<eepp/scene/mainthreadexecutor.hpp>` | `MainThreadExecutor`: runs continuations on the UI thread |
| `<eepp/core/uri.hpp>` | `URI` (formerly `Network::URI`, which remains an alias) |

Link the module with `eepp_module_vfs_add()` in Premake and include `<eepp/vfs.hpp>`. The UI model
lives in `<eepp/vfs/ui/vfsdirectorymodel.hpp>` and is not part of the aggregate header.

`VFSPath::validURI(uri)` rejects embedded NULs in decoded paths and authorities before native
calls. `VFSPath::validName(name, directory)` checks a single basename before appending untrusted
names: empty names, dot entries, slashes and NULs are invalid. Local Windows destinations also
reject backslashes, reserved characters and device names. Remote POSIX names retain their own
rules on Windows; transfer scans validate them again against the destination's rules.

The default is `eepp-vfs-static`; generate with `--with-shared-vfs` to build and link `eepp-vfs`
as a shared library. Public types and non-inline functions use `EE_VFS_API`, controlled by
`EE_VFS_EXPORTS` for the DLL and `EE_VFS_STATIC` for static builds, independently of core eepp
exports. The Premake helper sets the consumer flags and dependencies. The MinGW helper accepts
`--with-shared-vfs` as well. Combining it with `--with-static-eepp` is rejected: a shared VFS
must use the same eepp DLL as the application, including its resource and UI singletons.

## Tasks, results and threads

Every VFS operation returns `VFSTask<T>`, an alias of `Task<Result<T, VFSError>>`:

```cpp
auto& vfs = EE::VFS::VirtualFileSystem::instance();
MainThreadExecutor ui( sceneNode );

vfs.stat( URI( "sftp://makyo/home/black/notes.txt" ) )
	.then( ui, []( VFSResult<VFSFileInfo>&& info ) {
		if ( info )
			label->setText( info.value().name );
		else
			label->setText( info.error().toString() );
	} );
```

Rules:

- VFS methods never block and may be called from any thread.
- Tasks resolve on VFS threads. `then( fn )` runs where the task resolves; `then( executor, fn )`
  always goes through the executor. Touch UI or model state only from `then( uiExecutor, ... )`.
- A `Task` has one consumer: continuations consume it (`std::move( task ).then( ... )`).
  A continuation returning another task is flattened.
- If a producer is destroyed without resolving, its task chain becomes `Abandoned` instead of
  hanging. That is an infrastructure bug (reported by `TaskDiagnostics`), never a domain outcome.
- Cancellation is cooperative: pass `CancellationToken`s; cancelled operations complete with
  `VFSErrorCode::Cancelled`.
- `Task::wait()`/`get()` block. They exist for worker-thread adapters and tests; never use them
  on the UI thread.

Errors are typed (`VFSErrorCode`): callers branch on codes such as `NotFound`,
`PermissionDenied`, `ChangedExternally` or `HostKeyRejected`, never on errno or libssh2 values.

### Network mounts

A `file://` path can be on a network mount (NFS, SMB, SSHFS...). When its server stops answering,
a `hard` NFS mount blocks every call on it until the server is back, and nothing can interrupt the
blocked thread. `LocalIOScheduler` keeps that from spreading:

- Calls on local disks run on the I/O pool (`Config::ioThreads`). Calls under a network mount run
  on that mount's own lane (two threads), so a dead server never holds the threads every local
  folder needs. Mounts are found from the mount table, which never blocks.
- A path through a symbolic link into a mount is on that mount too. While any network mount is
  mounted, the provider first reads the path's links on the I/O pool (they are on local disks),
  stopping where the path enters a mount, then runs the call on that mount's lane. Watches and
  renames follow the links the same way.
- A call that takes longer than `Config::networkMountTimeout` (15 seconds by default) fails with
  `VFSErrorCode::TimedOut` ("server:/export is not responding"), and so does every call queued
  behind it. Until the stuck call returns, further calls on that mount fail at once. When it
  returns, its result is discarded and the mount works again.
  A timed-out read never writes into the caller's buffer afterwards (it reads into one of its own,
  copied only while the caller waits), and a write works from a copy. Closing a directory whose
  read is stuck never waits: the read closes it when it ends.
- Watches under a network mount poll through the VFS (`PollingWatchBackend`), whose calls time out,
  instead of using efsw.
- Listing a local folder never asks a mount's server: a network mount inside it is listed as a
  folder without metadata, and a symbolic link into a mount without its target's type.
- A rename between a local path and a network mount, or between two mounts, fails without calling
  the system with `Unsupported` (it would cross file systems), so a move copies and removes
  instead.

Applications that run their own blocking work on a local path (reading a file header, a trash
operation) can run it where the VFS runs that path's calls, with the same protection:

```cpp
vfs.localIO()
	.submit<bool>( path, [path]() -> VFSResult<bool> { return Ok( isExecutable( path ) ); } )
	.then( ui, []( VFSResult<bool>&& executable ) { ... } );
```

`localIO().submit( path, ... )` chooses the lane from the path as written; use
`submitResolved( path, followLast, ... )` when the path may go through a link. On Windows, links
are not followed yet: only UNC paths and mapped network drives are routed to a lane.

## Files and directories

```cpp
// Incremental listing: batches with an explicit end-of-directory flag.
vfs.openDirectory( uri );    // VFSTask<DirectoryHandle>; then handle.read( 256 ) per batch

vfs.listAll( uri );          // convenience: the whole directory
vfs.readAll( uri );          // VFSTask<std::vector<Uint8>>
vfs.writeAll( uri, data );   // atomic by default, returns the new VFSFileInfo
vfs.makeDirs( uri );
vfs.remove( uri, RemoveOptions{ true } );   // recursive
vfs.rename( from, to, RenameOptions{ true } );
vfs.canonicalize( uri );     // resolves symlinks/aliases; URIs are never canonicalized implicitly
```

`readAll()` reserves a known file length but keeps each handle read bounded to 256 KiB. An SFTP
operation deadline therefore applies to each chunk, rather than the whole file; a single chunk
can still time out. Explicit size limits, including `UINT64_MAX`, are checked without overflow.

`FileHandle` uses explicit offsets (`read( offset, span )`, `write( offset, span )`).

`VFSFileInfo` is a plain value. Unknown metadata stays unknown (`std::optional` accessors). It does
not store its own URI, because listings hold many entries and the caller always knows the URI it
listed: use `info.uriIn( directory )` for a listed entry (`VFSDirectoryModel::entryURI( row )` in
the model). `VFSFileInfo::revision` is an opaque token used for conflict detection:

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
transfer.completion().then( uiExecutor, []( VFSResult<TransferResult>&& r ) { ... } );
transfer.cancel();
```

Moves use a native rename when both sides share a provider filesystem namespace and fall back to copy
plus removal of the entries that were completely copied; a source is never removed after a partial
transfer. The copy-and-remove fallback requires the source provider's `Remove` capability. Without
it (for example, on `zip://` and `res://`), the move returns `Unsupported` before modifying the
destination.

Copy-and-remove moves require nonempty source revisions. Each file is checked against its scanned
revision before copying and again immediately before removal. A detected change returns
`ChangedExternally`, retains the source and leaves any published destination copy available for
recovery. A provider without revisions returns `Unsupported`; use an explicit copy instead.
Revision checks are best effort: SFTP v3 revisions have second-resolution timestamps and size,
and a pathname can still change between the final stat and removal. This is not an atomic
conditional-delete operation. Native same-provider renames do not need these copy/delete checks.

Transfers resolve whether both authorities share a filesystem namespace. Equivalent local
authorities (`file:///` and `file://localhost/`) and SSH aliases sharing a resolved session are
checked using canonical paths before a destination can be modified. Self-transfers return
`InvalidOperation`. Unknown identity across authorities returns `Unsupported` conservatively;
every provider implements `sameFileSystem()` to resolve aliases or identify disjoint namespaces.
This contract also applies to same-authority transfers; authority text alone does not decide identity.
Namespace identity does not promise a rename across device boundaries.
Local overwrite renames compare native file identity (device/inode on POSIX, volume/file index
on Windows). Moving between hardlinks returns `InvalidOperation` and retains both names;
matching bytes or timestamps alone do not identify a file.

Separate SSH sessions are treated as different logical servers only when their connected TCP
endpoints differ and their verified host keys differ within the same key algorithm. Hostname
spellings alone are insufficient. Matching peers or keys, unobservable proxy/jump endpoints, and
incomparable key algorithms return `Unsupported` before destination writes. Transfers between
independently identified direct SSH servers remain supported. This cannot detect shared storage
(such as NFS) behind independent servers; remote moves retain the best-effort revision limitations
above, rather than providing a global physical-file identity guarantee.

Directory copies through one provider also compare canonical paths, resolving the nearest existing
destination ancestor before checking containment. This rejects a destination reached through a
symlink back into the source. Providers without canonicalization retain the lexical containment
check; concurrent alias changes remain outside this guarantee.

A successful completion with `result.partial()` means some entries were skipped. Inspect
`filesSkipped` and report a partial transfer to the user: existing files under `Skip`, directory
symlinks, dangling links and unsupported special files are left uncopied. Links to regular files
are copied as their contents. A move retains every skipped source entry.
During copy-and-remove moves, all child symlinks are skipped: copying their targets would not
reproduce the links, and removing a target could break another link before it is read.

File copies are written to a temporary sibling before publication. `Fail` and `Skip` also apply
when a destination appears while the copy is running. The `Overwrite` policy requires
`AtomicReplace`; otherwise the transfer returns `Unsupported` and preserves the old contents.
Creating a file with `Fail` or `Skip` requires `Rename`. Moving a symlink through the copy fallback
is unsupported; native same-filesystem renames still move the link itself.

`MergeDirectories` merges existing destination directories but reports `AlreadyExists` for files,
including files that appear during the copy. It supports copying an archive root into an existing
folder without overwriting files. The default `Fail` policy also rejects existing directories;
`Overwrite` and `Skip` merge directories and apply their respective policies to files.

Native renames also preserve an existing destination when safe replacement is unavailable.
On POSIX systems without a native no-replace rename, public no-overwrite `rename()` returns
`Unsupported` for a missing destination. High-level moves use the revision-checked copy/remove
fallback described above, including its final stat/unlink race; use copy without removal if source
writers cannot be coordinated and the source must never be deleted after a concurrent replacement.
Only the internal writer's exclusively created temporary siblings may use atomic hard-link
publication followed by unlinking the temporary name. Both names can briefly exist; if temporary
cleanup fails, the operation returns `IO` explaining that publication succeeded and both names may
remain. The destination is never removed to roll back that failure. File systems must support hard
links for this publication fallback. No link/unlink move of an arbitrary public source, or
existence-check-then-replacing-rename fallback, is used.

## Watching

```cpp
vfs.watch( uri, WatchOptions{ /*recursive*/ true } )
	.then( ui, [this]( VFSResult<WatchHandle>&& watch ) {
		mWatch = std::move( watch ).value();
		mWatch.onEvents( ui, [this]( WatchEventBatch&& batch ) { ... } );
	} );
```

Events are relative to the watched root and arrive in coalesced batches through the executor you
choose. `RescanRequired` means continuity was lost (missed native events, reconnection, overflow):
re-list the directory. `WatchHandle::status()` reports `Active`, `Polling`, `Reconnecting` or
`Failed`, so a broken remote watch never looks healthy.

The generic polling backend refuses an initial snapshot above 100,000 entries with `Unsupported`.
If an existing watch exceeds that limit, it reports `Failed` and one `RescanRequired` event, then
stops polling that scope. Register smaller scopes to restore coverage. Partial snapshots are never
reported as complete watches. Any local native event received during registration causes
`RescanRequired`. Early event payloads are not replayed because newer live events can arrive
concurrently with publication of the registration.

Backends:

- `file://`: efsw native watching. Directories on a network mount are polled through the VFS
  instead (see Network mounts).
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
Fast listings coalesce subsequent batches before merging them into the sorted rows; slow listings
publish new rows at the next batch after 50 ms, and the last batch always flushes pending entries.

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

`SFTPProviderConfig::operationTimeout` bounds each started operation (30 seconds by default),
including waits for socket readiness. This applies even when `ServerAliveInterval` is unset.
Expiration fails the entire session and all queued operations with `TimedOut`; a later request
establishes a new session. Set a longer duration for operations expected to take more time.

eefm opens files from SFTP and other non-local providers as local copies after an explicit prompt.
Edits are not uploaded automatically. Each open downloads to a fresh private cache folder; earlier
copies remain available, so reopening cannot silently discard local edits.

### efsw-agent

`efsw-agent` is part of efsw (`src/agent/efsw-agent.cpp`, protocol in
`include/efsw/agent_protocol.hpp`). It has no eepp dependency and only reports changes. Install it
on the server's `PATH`, or set `SFTPProviderConfig::watchAgentCommand`. The eepp build produces
`bin/efsw-agent` for the build host.

Agent/watch establishment allows five seconds before falling back to polling. Applications can
choose a different bound when registering the backend with
`provider->createWatchBackend( vfs, Milliseconds( 500 ) )`; SSH connection/authentication deadlines
remain independent.

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

## ZIP archives (`zip://`)

`ZipProvider` is registered by default. Build archive locations with `makeURI()` rather than
concatenating paths: the backing file URI and the path inside the archive are separate identities.
Locations use `zip:///path/archive.zip!/member`. The host, when present, identifies a local UNC
archive. Literal `!` characters in the backing path are doubled, so a directory named `packages!`
uses `packages!!/` before the archive boundary. Member names use ordinary URI escaping.
`withAppendedSegment()` appends a member; eefm handles crossing the archive root with Up.

```cpp
URI archive = URI::fromFSPath( "/home/me/downloads/package.zip" );
URI root = ZipProvider::makeURI( archive );
URI folder = root.withAppendedSegment( "docs" );

vfs.listAll( folder ).then( ui, []( VFSResult<std::vector<VFSFileInfo>>&& entries ) { ... } );
vfs.readAll( folder.withAppendedSegment( "README.txt" ) );
vfs.copy( folder, URI::fromFSPath( "/home/me/extracted-docs" ) );
```

Directories exist both for explicit directory entries (including empty directories) and implicitly
for the parents of files. Listings include uncompressed file sizes and entry timestamps. Permission bits are unspecified
(the archive API does not expose them), so copies receive the destination filesystem's defaults. Browsing
never registers an archive in `PackRegistry` or `PackFileIndex`, so archive contents do not become
application resources. Opening a missing archive reports `NotFound` and never creates a file.

The provider is read-only: creating, replacing, renaming or deleting entries returns `ReadOnly`.
Files stream through independent readers without extracting the entire entry into memory.
Sequential reads retain the decompression position; seeking backwards replays the compressed
prefix with bounded scratch memory. `Seek` is advertised; `RandomAccess` is not. CRC failures are
reported as `IO`, including when the final read requests exactly the remaining file size.

Changes to the backing file invalidate its cached index for new operations. Open handles keep
their original reader and metadata; modifying an archive in place can cause their reads to fail.
The normal VFS polling backend detects archive changes. Archives on network mounts use the local
I/O scheduler's mount isolation and timeouts.

Only `file://` backing archives are supported initially. Password-protected entries and unsupported
compression methods report `Unsupported`. ZIP symlink attributes are not followed: their stored
data is exposed as a regular file. Unsafe names (absolute paths, traversal components, backslashes
or drive/stream colons) are excluded. Indexes are limited to 262,144 entries including implicit
directories and a conservative 64 MiB metadata budget covering libzip's decoded directory,
nodes, stored paths and container growth. Oversized central directories are rejected before
decoding; budget violations return `InvalidOperation`. At most eight indexes are cached;
existing handles can retain older indexes.

Legacy InfoZIP archives whose 16-bit entry count wraps to zero with a nonempty central directory
(for example, exactly 65,536 entries) are intentionally rejected as inconsistent. A zero count is
accepted only for an empty central directory.

In eefm, Enter or double-click on a local `.zip`, `.jar`, `.apk` or `.epub` file enters it
(extensions are case-insensitive). These ZIP-based formats share `ZipProvider` and `zip://` locations.
The archive context menu offers **Open in eefm** for browsing and **Open with <application>**
for launching the named external application. The **Open With** submenu also opens files externally.
Files inside open through the usual
application flow, and copying entries out uses VFS transfers. Up at the archive root returns to its
containing folder and selects the archive. History and saved tabs retain archive locations. The
location bar displays a readable address such as
`zip:///home/me/downloads/package.zip!/docs`, which can also be typed or pasted there.

For lower-level archive access, construct `Zip archive(false)` to opt out of the resource registry
and call `archive.open(path, Zip::OpenMode::ReadOnly)`. `entryCount()`, `entry()` and `openStream()`
provide metadata and streaming access. Streams borrow the archive and must be destroyed before it;
callers sharing an archive across threads must serialize access. `open()` never creates a missing
file; writing starts explicitly with `create()`. The default constructor retains resource-pack
registration for the engine and other existing pack consumers.

## Runtime archives (`archive://`)

`ArchiveProvider` is registered by default but loads libarchive only when used, on an I/O worker.
It requires a compatible libarchive **3.x** shared library, with no libarchive development headers
or build-time link dependency. Missing libraries, unsupported ABI versions and missing required
symbols return `Unsupported`; the rest of the VFS, including bundled ZIP support, remains available.
Emscripten and iOS have no runtime backend.

```cpp
URI backing = URI::fromFSPath( "/home/me/downloads/source.tar.gz" );
URI root = ArchiveProvider::makeURI( backing );
vfs.listAll( root );
vfs.copy( root.withAppendedSegment( "source" ), URI::fromFSPath( "/home/me/source" ) );

auto provider = std::dynamic_pointer_cast<ArchiveProvider>( vfs.provider( "archive" ) );
provider->support().then( ui, []( VFSResult<ArchiveSupport>&& support ) {
    // version, formats and compression filters from the installed runtime library.
} );
```

Locations use `archive:///path/source.tar.gz!/member`, with the same bang escaping, member path
rules and archive-root navigation as `zip://`. Only local `file://` backing archives are accepted.
An optional constructor `libraryPath` selects an exact library; otherwise the provider tries the
platform's usual libarchive names (including `/usr/lib/libarchive.2.dylib` on macOS).

Supported readers are enabled individually: TAR (including GNU and PAX variants), CPIO, ar, 7z,
RAR/RAR5, ISO9660, CAB, LHA, XAR and ZIP, when supplied by the loaded library. Compression filters
include gzip, bzip2, xz, LZMA, lzip, zstd and lz4 when built into that library. `support()` reports
the enabled set. External decompression programs are never enabled. Recognition does not guarantee
every entry can be decoded: encrypted or unsupported entry data and detected corruption report
errors when read. Integrity checks follow the installed libarchive; there is no extra checksum pass.
In particular, libarchive 3.8.2's gzip reader does not verify the gzip trailer CRC.

`eefm` recognizes `.tar`, `.tar.gz`, `.tgz`, other common compressed TAR suffixes, `.7z`, `.rar`,
`.iso`, `.cab`, `.cpio`, `.ar`, `.deb`, `.xar` and `.lha`/`.lzh` as browsing candidates. Extensions
are case-insensitive; the archive contents determine whether opening succeeds. A `.deb` opens its
outer ar container; nested archives currently need to be copied out before browsing. ZIP-based
formats continue to use the bundled `ZipProvider`.

Right-click a single recognized local archive in `eefm` for the **Extract** submenu. The first
action extracts into a folder named after the archive (for example `package.tar.gz` into
`package/`); **Extract Here** uses its containing folder. **Extract To...** accepts a destination
folder path or URI, including writable SFTP locations. Extraction uses VFS transfers with progress,
cancellation and confirmation before overwriting existing files. Hidden entries are included.
It follows VFS copy semantics: links to files are copied as contents, while directory symlinks and
unsupported special entries are skipped and reported as a partial transfer. Links are not recreated.

The provider is read-only; moves out fail before copying because source removal is unavailable.
Indexing scans headers and skips entry data without extracting it into memory. Compressed archives
may require decompression during indexing. Each opened file has an independent streaming reader;
opening a late entry or seeking backwards can replay the archive prefix, so repeated random reads
can be expensive. For plain and gzip TAR, an explicit file close returns one idle reader to its
index: opening later entries can continue forward, avoiding repeated prefix decompression during
sequential directory copies. Other codecs and formats are released on close, so large decoder
windows or solid blocks are not retained by the index. `Seek` is supported, `RandomAccess` is not.
Idle readers close their native files; reuse reopens at the saved input position and checks the
backing revision again. Cached decoder state therefore does not block deleting or replacing the
archive on Windows. Data is not cached as extracted files.

Symbolic and hard links resolve only inside the archive, with a 40-link limit; native filesystem
targets are never followed. Device nodes and FIFOs can be listed but cannot be opened as files.
Unsafe member names are excluded using the same checks as ZIP. Indexes have a 262,144-entry limit
and a conservative 64 MiB metadata budget, including implicit directories and stored paths, with
at most eight cached indexes. These limits cover the provider's index, not libarchive's own decoder
working memory. Open handles can retain older indexes.

Backing-file revisions invalidate cached indexes for new operations. Existing readers retain their
opened file and library; reopening for a backward seek after replacement reports `ChangedExternally`.
In-place modifications may cause read errors. Network mounts use the local scheduler's isolated
lanes and buffer-safe timeouts, and archive changes can be watched through the VFS polling backend.

## Resources (`res://`)

`System::PackFileIndex` (formerly `System::VirtualFileSystem`) indexes the files of every open `Pack`. `res:///fonts/a.ttf` maps to the pack path
`fonts/a.ttf`; directories are implicit. The provider is read-only and extracts files to memory
when they are opened.
