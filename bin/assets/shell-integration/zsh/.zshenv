# Restore the caller's ZDOTDIR before reading their configuration. Record changes it makes
# so login profiles and .zshrc are loaded from the same location zsh would normally use.
if [[ $ETERM_ZDOTDIR_SET == 1 ]]; then
    export ZDOTDIR=$ETERM_ORIGINAL_ZDOTDIR
else
    unset ZDOTDIR
fi
unset ETERM_ZDOTDIR_SET ETERM_ORIGINAL_ZDOTDIR
[[ ! -r ${ZDOTDIR-$HOME}/.zshenv ]] || source "${ZDOTDIR-$HOME}/.zshenv"
if [[ -o interactive ]]; then
    if (( ${+ZDOTDIR} )); then
        export ETERM_ZDOTDIR_SET=1 ETERM_ORIGINAL_ZDOTDIR=$ZDOTDIR
    else
        export ETERM_ZDOTDIR_SET=0
    fi
    ZDOTDIR=$ETERM_INTEGRATION_DIR/zsh
fi
