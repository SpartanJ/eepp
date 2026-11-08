# Shell adapter only. Recovery decisions and foreground execution belong to eterm.
set -q ETERM_HARNESS_DIRECTORY; and test -n "$ETERM_HARNESS_DIRECTORY"; or return
set -q ETERM_HARNESS_BINARY; and test -n "$ETERM_HARNESS_BINARY"; or return
command "$ETERM_HARNESS_BINARY" --harness-session attach $fish_pid; or return
set -gx ETERM_HARNESS_SHELL_PID $fish_pid

if not functions -q codex
    function codex --wraps=codex
        command "$ETERM_HARNESS_BINARY" --harness-session codex -- $argv
    end
end

function __eterm_clear_harness_session --on-event fish_prompt
    if test -s "$ETERM_HARNESS_DIRECTORY/active.json"
        printf '' > "$ETERM_HARNESS_DIRECTORY/active.json"
    end
end

if set -q ETERM_RESUME_SESSION; and test -n "$ETERM_RESUME_SESSION"
    command "$ETERM_HARNESS_BINARY" --harness-session resume
    set -e ETERM_RESUME_SESSION
end
