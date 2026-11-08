# Foreground processes and atomic local files

`Process::InheritStdio` borrows the caller's standard input/output/error handles when creating a
child. It creates no pipes: the three stream accessors return `nullptr`, and pipe-reading APIs
return no data. Combining it with `EnableAsync` or `CombinedStdoutStderr` fails at creation.
`NoWindow` is ignored for inherited terminal handles.

For a CLI forwarding its terminal to an interactive program, use `Process::runForeground()` with
an executable and an argument vector. It inherits the environment plus explicit overrides and
returns the child's exit code (127 if creation fails, or 128 + signal for a signal-terminated POSIX child). Arguments do not pass through a shell.
The caller can supply a working directory and a shutdown grace period, which defaults to two
seconds. This blocks the calling thread and installs temporary process-wide signal handlers;
only one foreground call may be active, and it belongs on a CLI main thread.

On POSIX, Ctrl-C remains with the terminal's foreground process group; TERM/HUP received by the
wrapper are forwarded to the child. On Windows the child shares the caller's console, which
delivers Ctrl-C, Ctrl-Break and close events to it directly. Ctrl-C never starts the
forced-shutdown timer. Stop events start the grace period, after which an unresponsive child is
killed. Windows does not send logoff or shutdown console events to a process that has loaded
`user32.dll`, which any eepp program has, so the grace period is not guaranteed for those two; the
child still receives them unless it loaded `user32.dll` too.
Previous signal handlers are restored before returning. `Process::requestStop()` separately requests
SIGTERM or console break without waiting; Windows callers must create that child with
`NewProcessGroup`. These native operations are unavailable on Emscripten.

`FileSystem::fileWriteAtomic(path, bytes, sync)` exclusively creates a randomly named sibling,
checks the complete write and close, and replaces the destination. Failure removes the temporary
file and preserves the destination. New POSIX files are owner-only; existing POSIX permissions are
retained. `sync=true` flushes file contents before replacement; it does not promise directory-entry durability after power loss.
Existing symlinks are replaced, so callers that intend to follow them should resolve their target
first, as `IniFile::writeFileAtomic()` does. Providers with their own transaction or streaming
semantics, such as VFS and image encoders, retain those semantics.

`FileSystem::createTempDirectory(prefix)` returns an exclusively created directory under the user's
temporary directory, or empty on failure. POSIX permissions are 0700; Windows uses an owner-only
DACL using the [Owner Rights SID](https://learn.microsoft.com/en-us/windows/win32/secauthz/sid-strings).
The caller owns removal. Prefixes cannot contain separators or NUL. On Windows,
`isRelativePath()` treats UNC paths and any drive-qualified path as not relative. That includes
drive-relative paths such as `C:foo`, which existing callers rely on.

`Sys::getProcessInfo(pid, includeArguments)` returns native process identity and parent information.
Exact arguments are optional and only returned when the platform exposes them; unavailable fields
remain empty. In particular, Haiku's shortened team argument summary is never presented as an exact
argument vector. `Sys::getRandomBytes()` fills caller-owned storage from OS cryptographic randomness
and fails if that facility is unavailable. `String::constantTimeEquals()` compares equal-length byte
strings without a content-dependent early exit; lengths are public.
