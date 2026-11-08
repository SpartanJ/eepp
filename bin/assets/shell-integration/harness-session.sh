# Bash/zsh adapter only. Recovery decisions and foreground execution belong to eterm.
[[ -n ${ETERM_HARNESS_DIRECTORY-} && -n ${ETERM_HARNESS_BINARY-} ]] || return
"$ETERM_HARNESS_BINARY" --harness-session attach "$$" || return
export ETERM_HARNESS_SHELL_PID=$$

if ! alias codex >/dev/null 2>&1 && ! typeset -f codex >/dev/null 2>&1; then
    codex() { command "$ETERM_HARNESS_BINARY" --harness-session codex -- "$@"; }
fi

__eterm_clear_harness_session() {
    local eterm_status=$?
    # Keep this in the shell: do not fork a helper on every prompt.
    if [[ -s $ETERM_HARNESS_DIRECTORY/active.json ]]; then
        : >| "$ETERM_HARNESS_DIRECTORY/active.json"
    fi
    return "$eterm_status"
}

if [[ -z ${__eterm_harness_attached-} ]]; then
    __eterm_harness_attached=1
    __eterm_add_prompt_hook __eterm_clear_harness_session
fi
if [[ -n ${ETERM_RESUME_SESSION-} ]]; then
    "$ETERM_HARNESS_BINARY" --harness-session resume
    unset ETERM_RESUME_SESSION
fi
