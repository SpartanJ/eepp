# eterm shell integration

eterm and ecode expose these options under **Settings → Terminal → Behavior**. They are enabled
by default and saved in the `[terminal]` section of each application's `config.cfg`:

```ini
[terminal]
inherit_working_directory = true
enable_osc8_hyperlinks = true
allow_osc52_clipboard_write = true
grapheme_clustering = true
```

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

## Working directories (OSC 7)

New tabs and splits inherit their source terminal's reported local directory. This requires the
shell or prompt to emit OSC 7 when its directory changes. A report looks like
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
