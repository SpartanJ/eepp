# Shared by bash and zsh. Work bytewise so non-ASCII UTF-8 and URI delimiters are escaped.
__eterm_report_working_directory() {
    local eterm_result=$?
    local LC_ALL=C eterm_directory="$PWD" eterm_encoded='' eterm_char eterm_hex eterm_index
    for ((eterm_index = 0; eterm_index < ${#eterm_directory}; eterm_index++)); do
        eterm_char=${eterm_directory:$eterm_index:1}
        case "$eterm_char" in
            [a-zA-Z0-9/._~-]) eterm_encoded+="$eterm_char" ;;
            *)
                # Bash 3.2 sign-extends bytes above 127; keep the byte, not that sign extension.
                printf -v eterm_hex '%d' "'$eterm_char"
                printf -v eterm_hex '%%%02X' "$((eterm_hex & 255))"
                eterm_encoded+="$eterm_hex"
                ;;
        esac
    done
    printf '\033]7;file://localhost%s\007' "$eterm_encoded"
    return "$eterm_result"
}

# Bash 5.0 accepts array assignments but only executes element zero. Array prompt hooks require
# 5.1; older Bash must retain the user's scalar command and the status seen by every hook.
__eterm_add_prompt_hook() {
    if [[ -n ${BASH_VERSION-} ]]; then
        if (( BASH_VERSINFO[0] > 5 || (BASH_VERSINFO[0] == 5 && BASH_VERSINFO[1] >= 1) )); then
            PROMPT_COMMAND=("$1" "${PROMPT_COMMAND[@]}")
        else
            PROMPT_COMMAND="$1${PROMPT_COMMAND:+; $PROMPT_COMMAND}"
        fi
    else
        autoload -Uz add-zsh-hook
        add-zsh-hook precmd "$1"
    fi
}
