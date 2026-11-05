# eterm shell integration

eterm and ecode expose these options under **Settings → Terminal → Behavior**. They are enabled
by default and saved in the `[terminal]` section of each application's `config.cfg`:

```ini
[terminal]
inherit_working_directory = true
enable_osc8_hyperlinks = true
allow_osc52_clipboard_write = true
grapheme_clustering = true
detect_links = true
```

## Shell exit behavior

In eterm, **Settings → Terminal → Behavior → Close Terminal Tab on Exit** is enabled by
default for new profiles. When the shell exits, including through Ctrl+D at an empty prompt,
its tab closes. Closing the last tab exits eterm.

Disable the option to restart a successfully exited shell in the same tab. A nonzero exit code
keeps the exited terminal visible so its output can be inspected. This policy also applies to an
explicitly configured shell. The setting is saved as `close_on_exit` in `[terminal]`; existing
saved values are honored, and `--close-on-exit` enables closing for the launch.

In ecode, the corresponding setting is `close_terminal_tab_on_exit` and defaults to disabled.
It uses the same close-or-restart policy for terminal tabs; closing one does not exit the editor.
The status-bar terminal panel always keeps at least one tab, creating a replacement when its
last tab closes.

## Mouse wheel and trackpads

Wheel input retains fractional movement on both axes. When an application captures the mouse,
eterm accumulates fractional wheel ticks and sends whole ticks using the application's mouse
protocol. Larger deltas can produce multiple reports, including when several input events arrive
in one UI frame. A traditional wheel tick still produces one report. Applications that use
alternate-screen scrolling without mouse capture receive cursor keys instead. Extremely large
events are limited to 128 reports per axis; excess whole ticks are discarded and the fractional
remainder is retained.

Shift overrides mouse capture for local selection and main-screen history scrolling. The existing
Ctrl+wheel font-size shortcuts remain available.

## Hyperlinks (OSC 8)

Programs can attach a target URI to text without printing the address:

```sh
printf '\033]8;;https://example.com/\033\\Example\033]8;;\033\\\n'
```

Use the default modifier with **Click** to open a link (**Ctrl** on Linux/Windows, **Cmd** on macOS). Right-click it for **Open Hyperlink**
and **Copy Hyperlink Address**. Opening supports `http`, `https` and `mailto`; other schemes can
be copied. Plain clicks and Shift+drag retain terminal mouse reporting and text selection behavior.
The existing **Open Link** action for URI-looking selected text remains available.

Hold the default modifier over a supported link to see a hand cursor, a thicker underline
and its target address at the bottom left of the terminal pane. The preview updates when the
modifier changes even if the pointer stays still. Long addresses are truncated to fit the pane.
The preview uses a thin top/right border with a rounded top-right corner and open left/bottom edges.
The application controls whether it prints a label alone or includes the URL as visible text;
eterm does not remove text sent by a terminal program.

Hyperlinks are enabled by default. Turning off **Enable Hyperlinks (OSC 8)** disables actions on
existing links and ignores new OSC 8 sequences in every open terminal. Turning it back on makes
previously recorded links available again; text printed while disabled has no link metadata.

## Links and paths in text

eterm also finds URLs and file paths in plain output, without any escape sequence. Hold the default
modifier over one to underline it and preview its target, then click to open it; right-click it
for **Open Link** and **Copy Link Address**. An OSC 8 hyperlink on the same text takes precedence.

Matching uses Ghostty's default link pattern:

- URLs with a scheme: `http`, `https`, `mailto`, `ftp`, `file`, `ssh`, `git`, `tel`, `magnet`,
  `ipfs`, `ipns`, `gemini`, `gopher` and `news`;
- absolute and relative paths: `/tmp/file`, `./build`, `../notes.md`, `~/.config/eterm`,
  `$HOME/src`, and paths with a directory and an extension such as `src/main.cpp`;
- Windows drive paths with either separator: `C:/Users/me/notes.md`, `D:\src\main.cpp`.

Text soft-wrapped across rows is matched as one line. A link is only offered when all of it is in
view: one that continues past the bottom of the viewport, or longer than about a kilobyte, is not
opened truncated. Relative paths resolve against the shell's
reported directory (OSC 7), or else the directory the terminal started in, and `~/` and a leading
`$VAR/` expand. A path is a link only if the file or directory exists; a trailing `:line` or
`:line:column`, as printed by compilers and `grep -n`, is ignored when needed to find it. URLs and
files open with the system's default application.

Turn off **Detect Links and Paths** to disable this in every open terminal.

## Working directories (OSC 7)

New tabs and splits inherit their source terminal's reported local directory. On Unix systems,
eterm automatically enables directory reporting for interactive Bash, zsh and fish sessions,
including the initial directory and subsequent `cd` commands. Bash and zsh retain the user's
startup files and prompt hooks; modern Bash/zsh login sessions and custom `ZDOTDIR` locations
are supported. Apple's legacy `/bin/bash` 3.2 login invocation is left unchanged; use an
ordinary interactive session or a current Bash for automatic reporting there.
Explicit commands/scripts and unsupported custom startup flags retain their original invocation.
PowerShell (`pwsh` and Windows `powershell.exe`) also reports its initial directory and each
subsequent prompt's directory. Profiles and existing prompts are retained, and `-NoProfile`,
`-NoLogo`, `-NoExit` and PowerShell login options are supported. Explicit commands, scripts and
other startup options retain their original invocation. Reporting covers local filesystem
directories, excluding UNC shares and virtual providers such as `Env:`. Windows execution
policies are respected; automatic reporting requires permission to run the bundled script.
Set `ETERM_SHELL_INTEGRATION=0` before launching eterm to disable automatic integration.
Other shells or prompts must emit OSC 7 when their directory changes. A report looks like
`OSC 7 ; file://localhost/absolute/path ST`; URI-encode spaces and other special characters.
For Windows drives, use a URI such as `file://localhost/C:/Users/name`.

eterm accepts an empty host, `localhost`, or the local machine's hostname. Remote hosts, malformed
encoding, relative paths and embedded NUL are rejected. Before creating a child, eterm checks the
directory locally. Missing or inaccessible directories fall back to that terminal's launch directory,
then the configured launch default. This does not infer a remote SSH directory as a local path.

The initial terminals retain the configured or command-line launch directory. Launching an executable
or using an explicit execute-in-shell command retains its launch behavior. Disable **Inherit Working
Directory** to always use the configured launch default for new tabs and splits.

In ecode, these options apply to editor terminals and the status-bar terminal panel. When no source
directory is available or inheritance is disabled, ecode uses its configured project, current-file,
home, or custom directory preference. Explicit build/run and context-menu launch directories take
priority over inheritance.

## Clipboard writes (OSC 52)

Terminal programs, including programs running over SSH, can write text to the local system clipboard
using OSC 52. Writes are allowed by default. Turn off
**Allow Programs to Write to Clipboard (OSC 52)** to deny them in existing and future terminals.

Every xterm selection target (`c`, `p`, `q`, `s`, `0`–`7`, any combination, or an empty selector)
writes the system clipboard, so for example Neovim's `"*` register works. eterm has no separate
primary selection. Unknown targets, invalid Base64, embedded NUL and writes exceeding 256 KiB of
decoded text are ignored. Clipboard reads are unsupported; a read query never returns local
clipboard content.

## Program status (OSC 7501 and OSC 9;4)

Programs can report what they are doing without eterm reading their output or guessing from
activity. eterm supports two independent protocols:

```sh
# OSC 7501 (revision 0.4): records with state, optional kind, progress, app, title and msg.
msg=$(printf '%s' 'Approve deployment?' | base64 | tr -d '\n')
printf '\033]7501;state=working:app=demo:progress=40\033\\'
printf '\033]7501;state=blocked:kind=permission:app=demo:msg=%s\033\\' "$msg"
printf '\033]7501;state=done:id=build/test\033\\'
printf '\033]7501;state=clear\033\\'

# OSC 9;4: a single progress value. 0 clears, 1 normal, 2 error, 3 indeterminate, 4 paused/warning.
printf '\033]9;4;1;75\a'
printf '\033]9;4;0\a'
```

`OSC 7501;?` is answered with `OSC 7501;?`, which means the default state and kind sets. Reports
over a limit of the protocol, with invalid base64, invalid UTF-8 or control characters in `msg` or
`title`, or with an invalid `id` are discarded whole. eterm accepts up to 4095 bytes when a report
ends in BEL, and up to 4096 when it ends in `ESC \\`. Other `OSC 9` sequences, such as desktop
notifications, are not supported.

Records and progress are never merged. A terminal's single indicator is chosen like this:

- If any record exists, one record is shown. `blocked` wins, then `error`, `working`, `done` and
  `idle`; the latest update decides between equal states.
- Otherwise OSC 9;4 progress is shown. After an OSC 7501 report, this needs a new OSC 9;4 report
  that arrives while no record exists: clearing the records doesn't bring back older progress.

| Event | OSC 7501 records | OSC 9;4 progress |
| --- | --- | --- |
| New prompt (OSC 133 `A`) or process exit | Keeps `done` and `error` only | Cleared |
| No new report for 15 seconds | Kept: they need no heartbeat | Cleared |
| Full reset (RIS) or restart | Cleared | Cleared |
| Soft reset (DECSTR) or screen switch | Kept | Kept |

With `close_on_exit`, a tab whose shell exits closes as usual, status included.

In eterm, the status replaces the tab's icon, a reported percentage draws a bar along the tab's
bottom edge, and the details appear in the tab's tooltip, without changing the title. A `done` or
`error` that arrives while its terminal isn't the focused one in the focused window also colors the
tab's title until you focus that terminal. Then `done` returns to the normal icon, and `error` keeps
its icon until the program clears it. Seeing a result changes only the tab: the program's record
stays, and nothing is sent to the program. `working` and `blocked` always show. `show_program_status` in `[terminal]` (**Show Program Status** in Settings →
Terminal) hides it, and doesn't change what programs can report or query.

Applications that embed the terminal get `TerminalDisplay::getProgramStatus()`, and
`EventType::PROGRAM_STATUS_CHANGE` on the UI thread. A snapshot is immutable, holds both protocols'
state, and offers `presentation` with the selection above. Its presented text has bidi overrides
and invisible formatting removed, but remains untrusted: never treat it as markup or a command.

## Grapheme clusters (mode 2027)

A character can be written as several codepoints: a letter followed by combining accents, an emoji
with a skin-tone modifier or variation selector, a family joined with zero-width joiners, or a
flag made of two regional indicators. eterm keeps such a grapheme cluster in one cell (two for wide
characters), so copying, searching and word selection return it whole.

Clusters are shaped with the terminal font and its fallback fonts, so accents, Indic conjuncts,
emoji with skin tones, emoji ZWJ sequences and flags are drawn the way the font combines them. When
the font would draw a cluster wider than its cells (for example an emoji ZWJ sequence it does not
know, which falls apart into separate emoji), eterm draws the cluster's first codepoint instead.

eterm segments text with the [UAX #29](https://www.unicode.org/reports/tr29/) rules
by default (DEC private mode 2027) and gives each cluster one width:

- a cluster that contains another codepoint with its own width is two cells wide, so emoji ZWJ
  sequences and flags take two cells;
- a variation selector 16 (U+FE0F) after an emoji makes it two cells wide, and a variation selector
  15 (U+FE0E) makes it one cell wide;
- a zero-width codepoint that does not continue the previous cluster is ignored.

Programs that measure text with `wcwidth()` count such clusters per codepoint, so a line editor can
misplace its cursor on a line that contains them. A program can turn the mode off with
`printf '\033[?2027l'` (and back on with `\033[?2027h`); a reset restores the configured mode. Turn
off **Grapheme Clustering** under **Settings → Terminal → Behavior** to start every terminal with
the mode off. eterm then keeps the widths `wcwidth()` computes: zero-width codepoints such as
combining marks join the previous character, every other codepoint takes its own cells, and emoji
variation selectors join only emoji pictographs.

Check support with DECRQM: `printf '\033[?2027$p'` is answered with `\033[?2027;1$y` while the
mode is on and `\033[?2027;2$y` while it is off. Clusters keep at most 32 codepoints; further
codepoints are dropped.

Protocol references: [OSC 8](https://gist.github.com/egmontkob/eb114294efbcd5adb1944c9f3cb5feda),
[OSC 7 shell integration](https://wezterm.org/shell-integration.html),
[Ghostty clipboard defaults](https://ghostty.org/docs/config/reference#clipboard-write) and
[mode 2027](https://github.com/contour-terminal/terminal-unicode-core).
