# Ordinary sessions use --rcfile, including macOS's Bash 3.2. Modern login sessions use ENV
# in POSIX startup mode, then restore normal Bash behavior before loading startup files.
[[ $- == *i* ]] || return
if [[ ${ETERM_BASH_POSIX_BOOTSTRAP-} == 1 ]]; then
    if [[ ${ETERM_BASH_ENV_SET-} == 1 ]]; then
        export ENV="$ETERM_BASH_ORIGINAL_ENV"
    else
        unset ENV
    fi
    unset ETERM_BASH_ENV_SET ETERM_BASH_ORIGINAL_ENV ETERM_BASH_POSIX_BOOTSTRAP
    set +o posix
    shopt -u inherit_errexit 2>/dev/null
fi
__eterm_startup_dir=$ETERM_INTEGRATION_DIR
__eterm_noprofile=$ETERM_BASH_NOPROFILE
__eterm_norc=$ETERM_BASH_NORC
__eterm_rcfile=$ETERM_BASH_RCFILE
unset ETERM_INTEGRATION_DIR ETERM_BASH_NOPROFILE ETERM_BASH_NORC ETERM_BASH_RCFILE
if shopt -q login_shell; then
    if [[ $__eterm_noprofile != 1 ]]; then
        [[ ! -r /etc/profile ]] || source /etc/profile
        for __eterm_profile in "$HOME/.bash_profile" "$HOME/.bash_login" "$HOME/.profile"; do
            if [[ -r $__eterm_profile ]]; then source "$__eterm_profile"; break; fi
        done
    fi
elif [[ $__eterm_norc != 1 ]]; then
    [[ -n $__eterm_rcfile ]] || __eterm_rcfile="$HOME/.bashrc"
    [[ ! -r $__eterm_rcfile ]] || source "$__eterm_rcfile"
fi
if [[ ${ETERM_SHELL_INTEGRATION-1} != 0 ]]; then
    source "$__eterm_startup_dir/directory-reporting.sh"
    # Preserve the existing command's status, and support modern PROMPT_COMMAND arrays.
    if (( BASH_VERSINFO[0] >= 5 )); then
        PROMPT_COMMAND=(__eterm_report_working_directory "${PROMPT_COMMAND[@]}")
    else
        PROMPT_COMMAND="__eterm_report_working_directory${PROMPT_COMMAND:+; $PROMPT_COMMAND}"
    fi
fi
unset __eterm_startup_dir __eterm_noprofile __eterm_norc __eterm_rcfile __eterm_profile
