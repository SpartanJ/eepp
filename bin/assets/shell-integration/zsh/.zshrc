if [[ $ETERM_ZDOTDIR_SET == 1 ]]; then
    export ZDOTDIR=$ETERM_ORIGINAL_ZDOTDIR
else
    unset ZDOTDIR
fi
unset ETERM_ZDOTDIR_SET ETERM_ORIGINAL_ZDOTDIR
typeset __eterm_startup_dir=$ETERM_INTEGRATION_DIR
unset ETERM_INTEGRATION_DIR
# macOS /etc/zshrc derives HISTFILE from the temporary ZDOTDIR before this shim runs.
# Restore that default to the user's directory before loading their own overrides.
if [[ $HISTFILE == "$__eterm_startup_dir/zsh/.zsh_history" ]]; then
    HISTFILE=${ZDOTDIR:-$HOME}/.zsh_history
fi
[[ ! -r ${ZDOTDIR-$HOME}/.zshrc ]] || source "${ZDOTDIR-$HOME}/.zshrc"
if [[ ${ETERM_SHELL_INTEGRATION-1} != 0 ]]; then
    source "$__eterm_startup_dir/directory-reporting.sh"
    autoload -Uz add-zsh-hook
    add-zsh-hook precmd __eterm_report_working_directory
    add-zsh-hook chpwd __eterm_report_working_directory
fi
unset __eterm_startup_dir
