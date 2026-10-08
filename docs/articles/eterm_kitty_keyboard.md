# eterm Kitty keyboard protocol

eterm implements Kitty keyboard protocol progressive enhancement. Applications negotiate it at
runtime with `CSI ? u`, `CSI > flags u`, `CSI < count u`, and `CSI = flags ; mode u`; changing
`TERM` to `xterm-kitty` is neither necessary nor recommended.

The implementation accepts all standardized flags. Main and alternate screens have independent
bounded mode stacks. Alternate-key reporting derives the standard PC-layout key from EEPP's
layout-independent scancode and the shifted key from committed text. Associated committed text is
reported when available from text input.

SDL2 and SDL3 repeat markers are preserved, so press, repeat, and release events are distinct when
event reporting is enabled. IME pre-edit input remains local; only committed text is forwarded.

For a raw diagnostic, an application can push report-all mode with `ESC [ > 8 u`. In that mode,
Ctrl+Enter is sent as `ESC [ 13 ; 5 u`. It must pop the mode with `ESC [ < u` before exiting.
Intermediaries such as tmux or screen must independently support and forward the protocol.
