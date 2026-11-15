# Environment variables

This is the lookup reference for environment variables read by eepp's framework and bundled
terminal and VFS modules. It also lists application profile overrides and the external driver and
developer-tool controls commonly used with eepp. Third-party libraries can expose additional
variables; their complete lists belong to their own documentation.

Set variables before launching the application. Some values are cached on first use, and changing
an environment variable does not recreate existing windows, fonts, or services. Names and symbolic
values are case-sensitive unless a row says otherwise. Defaults below describe the behavior when
the variable is unset; an application can supply its own settings where indicated.

## Runtime and window creation

| Variable | Values and default | Effect and scope |
| --- | --- | --- |
| `EEPP_RUNTIME` | `native` (default), `headless`, `terminal`. Unrecognized or empty values select `native`. | Selects execution mode on first use. `headless` renders offscreen; `terminal` presents the offscreen GUI through the Kitty graphics protocol and currently supports one top-level window. Offscreen runtimes still create a GL context; they do not imply software rendering. |
| `EEPP_WINDOW_HIDDEN` | Exactly `1` enables it; all other values leave visibility to the application. | Forces windows created through `Engine::createWindow()` hidden, including secondary windows. Suppresses native show, raise, restore, maximize, and fullscreen presentation. Keeps the selected renderer and runtime. A hidden native window still needs a usable native display. |
| `EEPP_MSAA` | Decimal integer `0` through `2147483647`. Default: application's requested sample count. | Overrides MSAA for every window before context creation. `0` disables MSAA; positive values request that sample count, subject to driver support. Empty, malformed, signed, or out-of-range values preserve the application request. Does not edit configuration files. |
| `EEPP_FORCE_SOFTWARE_RENDERING` | Nonempty values other than exactly `0` enable it. Default: no override. | Both SDL2 and SDL3 request a software GL visual with `SDL_GL_ACCELERATED_VISUAL=0` before window creation. Availability depends on the platform/driver. Does not select a particular software driver or GLX vendor; Mesa/llvmpipe may also require the external controls below. |

The first three controls are implemented by [Runtime](../../include/eepp/window/runtime.hpp)
and applied during [window creation](../../src/eepp/window/engine.cpp). The software visual
request is applied by both [SDL2](../../src/eepp/window/backend/SDL2/windowsdl2.cpp) and
[SDL3](../../src/eepp/window/backend/SDL3/windowsdl3.cpp) window creation.
See the [runtime inspector guide](ui_inspector.md) for hidden-window automation.

Software visual availability is separate from the renderer's implementation. Some X11/GLX
displays have no visual matching `SDL_GL_ACCELERATED_VISUAL=0`, so enabling
`EEPP_FORCE_SOFTWARE_RENDERING` can fail with `Couldn't find matching GLX visual` even when Mesa
llvmpipe is installed. On such a display, select Mesa/llvmpipe with the
[external driver controls](#external-driver-controls) while leaving this variable unset, or use a
supported offscreen runtime.

```sh
# Keep the application's saved MSAA preference, but disable it for this launch.
EEPP_MSAA=0 ./bin/eeiv

# Request four samples for this launch.
EEPP_MSAA=4 ./bin/eeiv

# Native rendering without opening desktop windows.
EEPP_WINDOW_HIDDEN=1 ./bin/eeiv

# Offscreen rendering; the available driver determines whether the renderer uses the GPU.
EEPP_RUNTIME=headless EEPP_MSAA=0 ./bin/eeiv
```

## UI scaling, fonts, and color scheme

| Variable | Values and default | Effect and scope |
| --- | --- | --- |
| `EEPP_PIXEL_DENSITY` | Positive finite floating-point scale, such as `1`, `1.5`, or `2`. Default: application/display selection. Empty, invalid, zero, negative, and nonfinite values are ignored. | Supplies UI pixel density when no explicit positive application/window density was supplied. `UIApplication` checks `Settings::pixelDensity`, then `WindowSettings::PixelDensity`, then this variable; its final fallback is display detection or `1` offscreen. |
| `EEPP_FONT_HINTING` | Exactly `none`, `slight`, or `full`. Default: application's choice; the standard `FontService` default is `full`. | Overrides hinting in font-service and TrueType font setters. Invalid values preserve the requested setting. |
| `EEPP_FONT_ANTIALIASING` | Exactly `none`, `grayscale`, or `subpixel`. Default: application's choice; the standard `FontService` default is `grayscale`. | Overrides font rasterization in font-service and TrueType font setters. Invalid values preserve the requested setting. This is independent of window MSAA. |
| `EEPP_COLOR_SCHEME` | Exactly `light`, `dark`, or `system`. Default: application's scene preference. | Overrides UI scene color-scheme selection, including subsequent preference setters. `system` resolves the OS preference. Invalid values preserve the application preference. |

Readers: [PixelDensity](../../src/eepp/graphics/pixeldensity.cpp),
[UIApplication](../../src/eepp/ui/uiapplication.cpp),
[FontService](../../src/eepp/graphics/fontservice.cpp),
[Font](../../src/eepp/graphics/fonttruetype.cpp), and
[UISceneNode](../../src/eepp/ui/uiscenenode.cpp).
See [UI introduction](ui_introduction.md#environment-overrides) for usage.

```sh
EEPP_PIXEL_DENSITY=1.5 EEPP_COLOR_SCHEME=dark ./bin/eeiv
EEPP_FONT_HINTING=slight EEPP_FONT_ANTIALIASING=grayscale ./bin/ecode
```

## Input method

| Variable | Values and default | Effect and scope |
| --- | --- | --- |
| `EEPP_IME_LOCATION_UPDATE_INTERVAL_MS` | Nonnegative integer milliseconds; default `50`. Values above `65535` are clamped to `65535`; malformed or overflowing input uses the default. | Throttles native IME candidate-window/text-input rectangle updates while retaining the latest location. `0` removes the cooldown. Cached on first use. |

Reader: [InputMethod](../../src/eepp/window/inputmethod.cpp).

## Terminal GUI presentation

These controls affect `EEPP_RUNTIME=terminal`, not an ordinary terminal widget embedded in a GUI.
The presenter reads them when it initializes.

| Variable | Values and default | Effect and scope |
| --- | --- | --- |
| `EEPP_TERMINAL_PERSISTENT_UPDATES` | Boolean; default enabled when `TERM` equals `eterm` ignoring case, disabled otherwise. | Retains one Kitty root image and edits its frames instead of sending anonymous full-frame images. Enable manually only with a terminal supporting that update path. |
| `EEPP_TERMINAL_DAMAGE_UPDATES` | Boolean; same terminal-dependent default. | Sends changed image rectangles. Requires persistent updates to be enabled; otherwise this setting has no effect. |
| `EEPP_TERMINAL_ZLIB_LEVEL` | Integer `0` through `9`; default `1`. Invalid values use `1`. | Compression level for transmitted image data. `0` disables compression. Small payloads and payloads that do not compress sufficiently are sent uncompressed. |
| `TERM` | Inherited terminal type. | `eterm`, compared without case sensitivity, enables the two update optimizations by default. Other values use the anonymous full-frame path by default. Does not itself enable terminal runtime. |

For these two boolean controls, `1`, `true`, `on`, and `yes` enable; `0`, `false`, `off`, and `no`
disable. Words are case-insensitive. Unset, empty, and unrecognized values use the default.

Reader: [KittyFramePresenter](../../src/eepp/window/kittyframepresenter.cpp).

```sh
EEPP_RUNTIME=terminal EEPP_MSAA=0 ./bin/eeiv
EEPP_RUNTIME=terminal EEPP_TERMINAL_ZLIB_LEVEL=0 ./bin/eeiv
```

## Accessibility

These are **presence flags**: any defined value, including empty, `0`, or `false`, enables the
behavior. Remove the variable from the environment to disable it.

| Variable | Default | Effect and scope |
| --- | --- | --- |
| `EEPP_DISABLE_ACCESSIBILITY` | Unset: follow the scene's accessibility policy. | Forces the null accessibility backend even if the application requests native accessibility. Read when a scene's backend is initialized. |
| `EEPP_ACCESSIBILITY_TRACE` | Unset: no tracing. | Enables native accessibility diagnostics such as client activity transitions and AT-SPI connection/dispatch details. Cached on first use. |
| `EEPP_ACCESSIBILITY_PROFILE` | Unset: no profiling. | Enables UI-thread dispatch timing for backends using the dispatch profiler, including AT-SPI and queued Windows UIA requests. Reports batches, message counts, and maxima to stderr with the `EEPP_A11Y_PROFILE` prefix; that prefix is output, not another environment variable. |

See [UI accessibility](ui_accessibility.md) and
[accessibility diagnostics](ui_accessibility_architecture.md#testing-and-debugging).
Readers: [AccessibilityManager](../../src/eepp/ui/accessibility/accessibilitymanager.cpp),
[trace control](../../src/eepp/ui/accessibility/accessibilitybackend.cpp), and
[dispatch profiler](../../src/eepp/ui/accessibility/accessibilitydispatchprofile.hpp).

## Runtime inspector

| Variable | Values and default | Effect and scope |
| --- | --- | --- |
| `EEPP_INSPECTOR` | Unset, empty, or exactly `0`: disabled. Any other nonempty value: enabled; use `1`. | Starts the inspector after a UI scene is registered. Creates an authenticated TCP interface that can inspect and control the application. |
| `EEPP_INSPECTOR_ADDRESS` | Default `127.0.0.1`. | Server bind address. Non-loopback binding exposes raw, unencrypted TCP and emits a warning. |
| `EEPP_INSPECTOR_PORT` | Integer `0` through `65535`; default `0`. | Server listen port; `0` asks the OS to choose. Invalid or empty supplied values prevent startup and emit an error. The Python client uses the same variable as its destination port. |
| `EEPP_INSPECTOR_READ_ONLY` | Same boolean rule as `EEPP_INSPECTOR`; default disabled. | Rejects all inspector `input.*` commands when enabled. |
| `EEPP_INSPECTOR_TOKEN` | Default: generate a token. | Server bearer token; a supplied empty token also selects generation. A generated token is printed once in the startup descriptor; a supplied token is omitted. The Python client reads this variable for authentication. |
| `EEPP_INSPECTOR_HOST` | Client default `127.0.0.1`. | **Python client only:** destination host for `projects/scripts/eepp-inspect.py`. Does not configure the server bind address. |

The Python client's `--host`, `--port`, and `--token` arguments override its environment defaults.
Use the emitted port when the server chooses one automatically.
See [runtime UI inspector](ui_inspector.md) for protocol, authentication, and automation examples.
Readers: [server](../../src/eepp/ui/tools/uiinspectorserver.cpp) and
[client](../../projects/scripts/eepp-inspect.py).

## Networking and certificates

| Variable | Default | Effect and scope |
| --- | --- | --- |
| `http_proxy`, `HTTP_PROXY` | No proxy from the environment. | `Http::getEnvProxyURI()` checks lowercase first, falling back to uppercase only when lowercase is absent. An empty lowercase value therefore suppresses uppercase fallback. A missing URI scheme is interpreted as `http://`. Applies to callers using this helper, not automatically to every socket. |
| `SSL_CERT_DIR` | Platform/bundled certificate discovery. | Directory searched during TLS initialization for `ca-bundle.crt`, `ca-certificates.crt`, `ca-root.crt`, `ca-bundle.pem`, then `cert.pem`. The first existing bundle is selected. If none is found, normal discovery continues. This eepp reader expects one directory containing a bundle, not a list of directories. |

Readers: [HTTP proxy helper](../../src/eepp/network/http.cpp) and
[TLS initialization](../../src/eepp/network/ssl/sslsocket.cpp).

## Paths, desktop integration, and packaging

These are standard environment inputs consumed by platform-specific helpers. Set directory
variables to absolute paths. Availability and fallbacks differ by platform and helper.

| Variable | Platform/default | Use in eepp |
| --- | --- | --- |
| `HOME` | Unix-like systems; supplied by the session. | User directory, `~` expansion, and platform config/cache/data fallbacks. macOS uses `Library/Application Support` and `Library/Caches`; Linux/BSD commonly use `.config`, `.cache`, and `.local/share`. |
| `USERPROFILE`, `HOMEDRIVE`, `HOMEPATH` | Windows. | User-directory discovery prefers `USERPROFILE`, then `HOMEDRIVE` plus `HOMEPATH`. `USERPROFILE` also supplies Windows `~` expansion when `HOME` is unavailable. |
| `APPDATA` | Windows. | Configuration and persistent application-data root. |
| `LOCALAPPDATA` | Windows. | Cache root. If unavailable, cache lookup falls back to the temporary directory. |
| `XDG_CONFIG_HOME` | Linux/BSD/Solaris; normally `$HOME/.config`. | Configuration root and desktop association/icon configuration lookup. `Sys::getConfigPath()` uses a defined value as supplied, including empty; it falls back to `HOME` only when the variable is absent. |
| `XDG_CONFIG_DIRS` | Linux/BSD; default `/etc/xdg`. | Colon-separated system desktop-association configuration directories. |
| `XDG_CACHE_HOME` | Linux/BSD/Solaris; default `$HOME/.cache`. | Cache root. The cache helper ignores empty and relative values. |
| `XDG_DATA_HOME` | Linux/BSD/Solaris/Haiku; normally `$HOME/.local/share`. | User application data; also used by desktop resource discovery. `Sys::getDataPath()` ignores empty and relative values. |
| `XDG_DATA_DIRS` | Colon-separated list; default `/usr/local/share:/usr/share`. | System data search paths. `Sys::getDataSearchPaths()` ignores empty and relative entries. |
| `TMPDIR` | Non-Windows/non-Android; default `/tmp`. | Temporary directory. Windows uses its native temporary-path API; Android uses the application's internal storage. |
| `PATH` | Inherited executable search path. | `Sys::which()`, process/tool discovery, and desktop helper lookup. |
| `PATHEXT` | Windows; inherited extension list. | Executable lookup and completion, including extensionless commands. Some lookup state is cached. |
| `XDG_CURRENT_DESKTOP` | Linux/BSD; inherited desktop name/list. | Desktop-specific association precedence, desktop-entry visibility, and icon-theme selection. |
| `GTK_THEME` | Linux/BSD color-scheme detection. | A value containing `dark`, ignoring case, selects dark appearance during OS preference detection; otherwise configuration-file detection continues. |
| `TERMINAL` | Linux/BSD; application/desktop fallback. | Terminal executable for launching desktop entries marked `Terminal=true`; framework fallback prefers `konsole`, then `xterm`. eefm also consults it for terminal actions. |
| `APPDIR` | Linux/BSD AppImage launcher input. | Packaged resource-root fallback when the executable's resource layout requires it and `$APPDIR/assets` exists. |
| `APPIMAGE` | Linux AppImage launcher input. | Original AppImage executable path used when preparing application file-association registrations. |
| `FLATPAK_ID` | Linux; unset outside Flatpak. | Its presence selects Flatpak-aware process handling. Detection is cached. |

Readers: [Sys](../../src/eepp/system/sys.cpp),
[FileSystem](../../src/eepp/system/filesystem.cpp),
[Applications](../../src/eepp/system/applications.cpp),
[file associations](../../src/eepp/system/fileassociation.cpp),
[icon lookup](../../src/eepp/ui/icons/freedesktopiconlookup.cpp), and
[Process](../../src/eepp/system/process.cpp).
See [application packaging](application_packaging.md) for resource layouts.

## Bundled terminal and VFS modules

| Variable | Values/default | Effect and scope |
| --- | --- | --- |
| `ETERM_LOG_ERRORS` | Release: unset, empty, or `0` disables diagnostics; any other nonempty value enables them. Debug builds always enable them. | Terminal emulator diagnostics for malformed or unsupported control sequences. Cached on first use. |
| `ETERM_SHELL_INTEGRATION` | Exactly `0` disables automatic integration; default enabled for supported interactive shells. | Controls automatic working-directory reporting. An explicit child-environment entry takes precedence over the parent environment. See [eterm integration](eterm_integration.md#working-directories-osc-7). |
| `SHELL` | Inherited shell selection. | Default shell when no explicit terminal command is selected; also the Unix VFS SSH command transport shell, where empty/unset falls back to `/bin/sh`. |
| `POSIXLY_CORRECT` | Unix shell integration; inherited. | Its presence in the child environment prevents the Bash integration wrapper from changing invocation semantics. |
| `ENV`, `ZDOTDIR` | User's shell startup settings. | Bash/zsh integration preserves these values while temporarily selecting its bundled startup wrapper. Generated `ETERM_BASH_*`/`ETERM_ZDOTDIR_*` bookkeeping is internal and should not be configured manually. |
| `SSH_AUTH_SOCK` | Agent socket path; Windows fallback is the OpenSSH agent named pipe. | VFS SSH configuration's default `IdentityAgent`, unless overridden or disabled by SSH configuration. |
| `PROGRAMDATA` | Windows; fallback `C:/ProgramData`. | VFS SSH system-wide known-hosts paths under `ssh/`. |

Terminal links can expand arbitrary `$NAME/path` prefixes. SSH configuration can expand
environment references in supported directives; these are lookups of user-selected names, not
additional fixed eepp settings. See [VFS](vfs.md) for the SSH configuration layer.

Readers: [terminal diagnostics](../../src/modules/eterm/src/eterm/terminal/terminalemulator.cpp),
[shell integration](../../src/modules/eterm/src/eterm/system/shellintegration.cpp),
[terminal links](../../src/modules/eterm/src/eterm/terminal/terminallinks.cpp), and
[SSH configuration](../../src/modules/vfs/src/eepp/vfs/ssh/sshconfig.cpp).

## Application profile directories

These belong to the standalone applications; they are not framework-wide settings.

| Variable | Default | Effect |
| --- | --- | --- |
| `ECODE_CONFIG_PATH` | ecode's normal configuration/profile selection. | Selects an ecode profile directory and enables its profile/portable path handling. An explicit `--profile` takes precedence. |
| `ETERM_CONFIG_PATH` | `Sys::getConfigPath("eterm")`. | Selects eterm preferences, window state, and keybindings directory. An explicit `--profile` takes precedence. |

Application-specific integrations, provider API keys, and session-relay bookkeeping are owned by
those applications rather than eepp's runtime. Readers for profile selection:
[ecode](../../src/tools/ecode/ecode.cpp), [eterm](../../src/tools/eterm/eterm.cpp).

## External driver controls

These variables are interpreted by SDL, Mesa, libglvnd, or the desktop session. Their availability
depends on the installed dependency, selected backend, and platform. They are useful alongside
eepp's own overrides, but do not have an eepp-defined global default.

| Variable | Owner and common use |
| --- | --- |
| `SDL_VIDEODRIVER` / `SDL_VIDEO_DRIVER` | SDL2 / SDL3 video driver, such as `x11`, `wayland`, or `offscreen` when supported. eepp sets both spellings to `offscreen` for its headless runtime and on platforms where its terminal runtime uses that driver. |
| `SDL_VIDEO_EGL_DRIVER` / `SDL_EGL_LIBRARY` | SDL2 / SDL3 EGL library path. On Linux offscreen runs, eepp also retains a reference to the selected library; fallback `libEGL.so.1`. |
| `SDL_AUDIODRIVER` / `SDL_AUDIO_DRIVER` | SDL2 / SDL3 audio backend. `dummy` is useful for silent tests with the SDL-based MojoAL backend. |
| `SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS` | Both backends default this hint to `0` when no value was supplied. Controls fullscreen-window minimization on focus loss where supported. |
| `SDL_FORCE_RAISEWINDOW` / `SDL_HINT_FORCE_RAISEWINDOW` | SDL3 / SDL2 spelling of the force-raise hint in the backend's SDL headers. eepp defaults it to `1` when available and not supplied. |
| `SDL_RETURN_KEY_HIDES_IME`, `SDL_IME_SHOW_UI`, `SDL_IME_SUPPORT_EXTENDED_TEXT` | SDL2 backend IME hints, defaulted to `1` when available and not supplied. Availability depends on the SDL2 version. |
| `SDL_VIDEO_ALLOW_SCREENSAVER`, `SDL_MOUSE_FOCUS_CLICKTHROUGH` | Set by the corresponding `DisplayManager` methods. Native `UIApplication` initialization enables both. |
| `SDL_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR` | Set by compositor-bypass methods when SDL exposes the hint. Native `UIApplication` initialization requests `0` to avoid bypassing the compositor. |
| `DISPLAY`, `WAYLAND_DISPLAY`, `XDG_RUNTIME_DIR` | Native display/session connection inputs. Relevant to SDL's X11/Wayland backends and their platform services. |
| `LIBGL_ALWAYS_SOFTWARE` | Mesa: set `1` to select software rendering. |
| `GALLIUM_DRIVER` | Mesa: `llvmpipe` selects the LLVM software renderer when using the software path. |
| `__GLX_VENDOR_LIBRARY_NAME` | libglvnd/GLX: `mesa` selects Mesa dispatch, useful when the normal GLX vendor is NVIDIA. |
| `LP_NUM_THREADS` | llvmpipe rendering-worker count. `0` disables rendering threading; otherwise use an integer worker count. Default follows the available CPU cores, subject to driver limits. |

Complete dependency references: [SDL3 hints](https://wiki.libsdl.org/SDL3/CategoryHints),
[Mesa environment variables](https://docs.mesa3d.org/envvars.html), and
[libglvnd](https://github.com/NVIDIA/libglvnd).
eepp sets some SDL hints itself, including an override for `SDL_QUIT_ON_LAST_WINDOW_CLOSE=0`;
dependency variables are not guaranteed to override application hints set at override priority.

```sh
# Linux/X11: force llvmpipe and avoid requesting an unsupported multisampled visual.
LIBGL_ALWAYS_SOFTWARE=1 __GLX_VENDOR_LIBRARY_NAME=mesa GALLIUM_DRIVER=llvmpipe EEPP_MSAA=0 ./bin/eeiv

# Silent release tests with either SDL backend's audio-variable spelling.
SDL_AUDIODRIVER=dummy SDL_AUDIO_DRIVER=dummy bin/unit_tests/eepp-unit_tests
```

## Developer and test controls

These affect tools or test harnesses, not ordinary application behavior.

| Variable | Reader and behavior |
| --- | --- |
| `EEPP_REQUIRE_SFTP_TESTS` | Unit tests: exactly `1` requires the local SFTP test fixture rather than allowing an unavailable fixture to be skipped. |
| `EEPP_REQUIRE_TLS_TESTS` | Unit tests: exactly `1` requires the local TLS fixture rather than allowing an unavailable fixture to be skipped. |
| `EEPP_TEST_RESOURCE_CWD` | Unit-test entry point: changes the caller working directory for packaged resource-discovery checks. |
| `EEPP_TEST_RESOURCE_PATH` | Resource-path unit test: expected resolved resource directory for a staged package. |
| `EEPP_TEST_RESOURCE_BINARY` | Packaging tests: native unit-test executable used for staged resource-path checks. |
| `EEPP_TEST_SDL3_DYLIB` | macOS packaging tests: SDL3 dylib for native SDK/app/DMG fixtures. |
| `EEPP_TEST_POWERSHELL` | Packaging tests: PowerShell executable; explicitly enables these checks on Unix. Otherwise Windows searches for `pwsh`, then `powershell`. |
| `EEPP_TEST_WINDOWS_APPS` | Packaging tests: comma-separated built application identifiers to package and check. |
| `EEPP_TEST_WINDOWS_BIN` | Packaging tests: source Windows binary/assets directory; default repository `bin/`. |

See [application packaging tests](application_packaging.md) for fixture setup. Variables generated
inside a fixture, or emitted by `bootstrap_dev_environment.sh` to construct compiler/library
search paths, are tool bookkeeping rather than runtime configuration.

## Keeping this reference current

When adding or changing an environment reader, update its row here with accepted values, fallback,
platform/backend scope, and when it is read. Preserve differences between exact `1`, nonempty
nonzero strings, and presence flags. Include variables passed indirectly to helper functions and
the inspector client's destination variables when auditing readers.

```sh
rg -n 'getenv|getEnv\(|getEnvSplit\(|_dupenv_s|GetEnvironmentVariable' src/eepp include/eepp src/modules
rg -n 'EEPP_|ETERM_LOG_ERRORS|ETERM_SHELL_INTEGRATION' src/eepp src/modules projects/scripts/eepp-inspect.py
```
