# bash completion for yc                                   -*- shell-script -*-

__yc_debug()
{
    if [[ -n ${BASH_COMP_DEBUG_FILE} ]]; then
        echo "$*" >> "${BASH_COMP_DEBUG_FILE}"
    fi
}

# Homebrew on Macs have version 1.3 of bash-completion which doesn't include
# _init_completion. This is a very minimal version of that function.
__yc_init_completion()
{
    COMPREPLY=()
    _get_comp_words_by_ref "$@" cur prev words cword
}

__yc_index_of_word()
{
    local w word=$1
    shift
    index=0
    for w in "$@"; do
        [[ $w = "$word" ]] && return
        index=$((index+1))
    done
    index=-1
}

__yc_contains_word()
{
    local w word=$1; shift
    for w in "$@"; do
        [[ $w = "$word" ]] && return
    done
    return 1
}

__yc_handle_reply()
{
    __yc_debug "${FUNCNAME[0]}"
    case $cur in
        -*)
            if [[ $(type -t compopt) = "builtin" ]]; then
                compopt -o nospace
            fi
            local allflags
            if [ ${#must_have_one_flag[@]} -ne 0 ]; then
                allflags=("${must_have_one_flag[@]}")
            else
                allflags=("${flags[*]} ${two_word_flags[*]}")
            fi
            COMPREPLY=( $(compgen -W "${allflags[*]}" -- "$cur") )
            if [[ $(type -t compopt) = "builtin" ]]; then
                [[ "${COMPREPLY[0]}" == *= ]] || compopt +o nospace
            fi

            # complete after --flag=abc
            if [[ $cur == *=* ]]; then
                if [[ $(type -t compopt) = "builtin" ]]; then
                    compopt +o nospace
                fi

                local index flag
                flag="${cur%=*}"
                __yc_index_of_word "${flag}" "${flags_with_completion[@]}"
                COMPREPLY=()
                if [[ ${index} -ge 0 ]]; then
                    PREFIX=""
                    cur="${cur#*=}"
                    ${flags_completion[${index}]}
                    if [ -n "${ZSH_VERSION}" ]; then
                        # zsh completion needs --flag= prefix
                        eval "COMPREPLY=( \"\${COMPREPLY[@]/#/${flag}=}\" )"
                    fi
                fi
            fi
            return 0;
            ;;
    esac

    # check if we are handling a flag with special work handling
    local index
    __yc_index_of_word "${prev}" "${flags_with_completion[@]}"
    if [[ ${index} -ge 0 ]]; then
        ${flags_completion[${index}]}
        return
    fi

    # we are parsing a flag and don't have a special handler, no completion
    if [[ ${cur} != "${words[cword]}" ]]; then
        return
    fi

    local completions
    completions=("${commands[@]}")
    if [[ ${#must_have_one_noun[@]} -ne 0 ]]; then
        completions=("${must_have_one_noun[@]}")
    fi
    if [[ ${#must_have_one_flag[@]} -ne 0 ]]; then
        completions+=("${must_have_one_flag[@]}")
    fi
    COMPREPLY=( $(compgen -W "${completions[*]}" -- "$cur") )

    if [[ ${#COMPREPLY[@]} -eq 0 && ${#noun_aliases[@]} -gt 0 && ${#must_have_one_noun[@]} -ne 0 ]]; then
        COMPREPLY=( $(compgen -W "${noun_aliases[*]}" -- "$cur") )
    fi

    if [[ ${#COMPREPLY[@]} -eq 0 ]]; then
        declare -F __custom_func >/dev/null && __custom_func
    fi

    # available in bash-completion >= 2, not always present on macOS
    if declare -F __ltrim_colon_completions >/dev/null; then
        __ltrim_colon_completions "$cur"
    fi

    # If there is only 1 completion and it is a flag with an = it will be completed
    # but we don't want a space after the =
    if [[ "${#COMPREPLY[@]}" -eq "1" ]] && [[ $(type -t compopt) = "builtin" ]] && [[ "${COMPREPLY[0]}" == --*= ]]; then
       compopt -o nospace
    fi
}

# The arguments should be in the form "ext1|ext2|extn"
__yc_handle_filename_extension_flag()
{
    local ext="$1"
    _filedir "@(${ext})"
}

__yc_handle_subdirs_in_dir_flag()
{
    local dir="$1"
    pushd "${dir}" >/dev/null 2>&1 && _filedir -d && popd >/dev/null 2>&1
}

__yc_handle_flag()
{
    __yc_debug "${FUNCNAME[0]}: c is $c words[c] is ${words[c]}"

    # if a command required a flag, and we found it, unset must_have_one_flag()
    local flagname=${words[c]}
    local flagvalue
    # if the word contained an =
    if [[ ${words[c]} == *"="* ]]; then
        flagvalue=${flagname#*=} # take in as flagvalue after the =
        flagname=${flagname%=*} # strip everything after the =
        flagname="${flagname}=" # but put the = back
    fi
    __yc_debug "${FUNCNAME[0]}: looking for ${flagname}"
    if __yc_contains_word "${flagname}" "${must_have_one_flag[@]}"; then
        must_have_one_flag=()
    fi

    # if you set a flag which only applies to this command, don't show subcommands
    if __yc_contains_word "${flagname}" "${local_nonpersistent_flags[@]}"; then
      commands=()
    fi

    # keep flag value with flagname as flaghash
    # flaghash variable is an associative array which is only supported in bash > 3.
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        if [ -n "${flagvalue}" ] ; then
            flaghash[${flagname}]=${flagvalue}
        elif [ -n "${words[ $((c+1)) ]}" ] ; then
            flaghash[${flagname}]=${words[ $((c+1)) ]}
        else
            flaghash[${flagname}]="true" # pad "true" for bool flag
        fi
    fi

    # skip the argument to a two word flag
    if __yc_contains_word "${words[c]}" "${two_word_flags[@]}"; then
        c=$((c+1))
        # if we are looking for a flags value, don't show commands
        if [[ $c -eq $cword ]]; then
            commands=()
        fi
    fi

    c=$((c+1))

}

__yc_handle_noun()
{
    __yc_debug "${FUNCNAME[0]}: c is $c words[c] is ${words[c]}"

    if __yc_contains_word "${words[c]}" "${must_have_one_noun[@]}"; then
        must_have_one_noun=()
    elif __yc_contains_word "${words[c]}" "${noun_aliases[@]}"; then
        must_have_one_noun=()
    fi

    nouns+=("${words[c]}")
    c=$((c+1))
}

__yc_handle_command()
{
    __yc_debug "${FUNCNAME[0]}: c is $c words[c] is ${words[c]}"

    local next_command
    if [[ -n ${last_command} ]]; then
        next_command="_${last_command}_${words[c]//:/__}"
    else
        if [[ $c -eq 0 ]]; then
            next_command="_yc_root_command"
        else
            next_command="_${words[c]//:/__}"
        fi
    fi
    c=$((c+1))
    __yc_debug "${FUNCNAME[0]}: looking for ${next_command}"
    declare -F "$next_command" >/dev/null && $next_command
}

__yc_handle_word()
{
    if [[ $c -ge $cword ]]; then
        __yc_handle_reply
        return
    fi
    __yc_debug "${FUNCNAME[0]}: c is $c words[c] is ${words[c]}"
    if [[ "${words[c]}" == -* ]]; then
        __yc_handle_flag
    elif __yc_contains_word "${words[c]}" "${commands[@]}"; then
        __yc_handle_command
    elif [[ $c -eq 0 ]]; then
        __yc_handle_command
    elif __yc_contains_word "${words[c]}" "${command_aliases[@]}"; then
        # aliashash variable is an associative array which is only supported in bash > 3.
        if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
            words[c]=${aliashash[${words[c]}]}
            __yc_handle_command
        else
            __yc_handle_noun
        fi
    else
        __yc_handle_noun
    fi
    __yc_handle_word
}


if ! type -t _get_comp_words_by_ref >/dev/null 2>/dev/null ; then
    return
fi

_yc_iam_role_get()
{
    last_command="yc_iam_role_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_role_list()
{
    last_command="yc_iam_role_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_role()
{
    last_command="yc_iam_role"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_get()
{
    last_command="yc_iam_service-account_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_list()
{
    last_command="yc_iam_service-account_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_create()
{
    last_command="yc_iam_service-account_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_update()
{
    last_command="yc_iam_service-account_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_delete()
{
    last_command="yc_iam_service-account_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_list-access-bindings()
{
    last_command="yc_iam_service-account_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_set-access-bindings()
{
    last_command="yc_iam_service-account_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_add-access-binding()
{
    last_command="yc_iam_service-account_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_remove-access-binding()
{
    last_command="yc_iam_service-account_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account_list-operations()
{
    last_command="yc_iam_service-account_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_service-account()
{
    last_command="yc_iam_service-account"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_certificate_get()
{
    last_command="yc_iam_certificate_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--federation-id=")
    flags+=("--federation-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_certificate_list()
{
    last_command="yc_iam_certificate_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--federation-id=")
    flags+=("--federation-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_certificate_create()
{
    last_command="yc_iam_certificate_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--federation-id=")
    flags+=("--federation-name=")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--certificate-file=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_certificate_update()
{
    last_command="yc_iam_certificate_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--federation-id=")
    flags+=("--federation-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--certificate-file=")
    flags_with_completion+=("--certificate-file")
    flags_completion+=("__yc_handle_filename_extension_flag pem|crt")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_certificate_delete()
{
    last_command="yc_iam_certificate_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--federation-id=")
    flags+=("--federation-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_certificate_list-operations()
{
    last_command="yc_iam_certificate_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--federation-id=")
    flags+=("--federation-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_certificate()
{
    last_command="yc_iam_certificate"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation_get()
{
    last_command="yc_iam_federation_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation_list()
{
    last_command="yc_iam_federation_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation_create()
{
    last_command="yc_iam_federation_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--issuer=")
    flags+=("--cookie-max-age=")
    flags+=("--sso-binding=")
    flags+=("--auto-create-account-on-login")
    flags+=("--sso-url=")
    flags+=("--encrypted-assertions")
    flags+=("--case-insensitive-name-ids")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation_update()
{
    last_command="yc_iam_federation_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--issuer=")
    flags+=("--cookie-max-age=")
    flags+=("--sso-binding=")
    flags+=("--auto-create-account-on-login")
    flags+=("--sso-url=")
    flags+=("--encrypted-assertions")
    flags+=("--case-insensitive-name-ids")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation_delete()
{
    last_command="yc_iam_federation_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation_add-user-accounts()
{
    last_command="yc_iam_federation_add-user-accounts"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--name-ids=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation_list-user-accounts()
{
    last_command="yc_iam_federation_list-user-accounts"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation_list-operations()
{
    last_command="yc_iam_federation_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_federation()
{
    last_command="yc_iam_federation"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("add-user-accounts")
    commands+=("list-user-accounts")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_key_get()
{
    last_command="yc_iam_key_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_key_list()
{
    last_command="yc_iam_key_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_key_create()
{
    last_command="yc_iam_key_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--output=")
    two_word_flags+=("-o")
    flags+=("--user-account")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--description=")
    flags+=("--algorithm=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_key_delete()
{
    last_command="yc_iam_key_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_key()
{
    last_command="yc_iam_key"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_access-key_get()
{
    last_command="yc_iam_access-key_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_access-key_list()
{
    last_command="yc_iam_access-key_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_access-key_create()
{
    last_command="yc_iam_access-key_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--description=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_access-key_delete()
{
    last_command="yc_iam_access-key_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_access-key()
{
    last_command="yc_iam_access-key"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_api-key_get()
{
    last_command="yc_iam_api-key_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_api-key_list()
{
    last_command="yc_iam_api-key_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_api-key_create()
{
    last_command="yc_iam_api-key_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--description=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_api-key_delete()
{
    last_command="yc_iam_api-key_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_api-key()
{
    last_command="yc_iam_api-key"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_user-account_get()
{
    last_command="yc_iam_user-account_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--login=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_user-account()
{
    last_command="yc_iam_user-account"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam_create-token()
{
    last_command="yc_iam_create-token"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iam()
{
    last_command="yc_iam"

    command_aliases=()

    commands=()
    commands+=("role")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("roles")
        aliashash["roles"]="role"
    fi
    commands+=("service-account")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("service-accounts")
        aliashash["service-accounts"]="service-account"
    fi
    commands+=("certificate")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("certificates")
        aliashash["certificates"]="certificate"
    fi
    commands+=("federation")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("federations")
        aliashash["federations"]="federation"
    fi
    commands+=("key")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("keys")
        aliashash["keys"]="key"
    fi
    commands+=("access-key")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("access-keys")
        aliashash["access-keys"]="access-key"
    fi
    commands+=("api-key")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("api-keys")
        aliashash["api-keys"]="api-key"
    fi
    commands+=("user-account")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("user-accounts")
        aliashash["user-accounts"]="user-account"
    fi
    commands+=("create-token")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud_get()
{
    last_command="yc_resource-manager_cloud_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud_list()
{
    last_command="yc_resource-manager_cloud_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud_update()
{
    last_command="yc_resource-manager_cloud_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud_list-access-bindings()
{
    last_command="yc_resource-manager_cloud_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud_set-access-bindings()
{
    last_command="yc_resource-manager_cloud_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud_add-access-binding()
{
    last_command="yc_resource-manager_cloud_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud_remove-access-binding()
{
    last_command="yc_resource-manager_cloud_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud_list-operations()
{
    last_command="yc_resource-manager_cloud_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_cloud()
{
    last_command="yc_resource-manager_cloud"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("update")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_get()
{
    last_command="yc_resource-manager_folder_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_list()
{
    last_command="yc_resource-manager_folder_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_create()
{
    last_command="yc_resource-manager_folder_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_update()
{
    last_command="yc_resource-manager_folder_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_add-labels()
{
    last_command="yc_resource-manager_folder_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_remove-labels()
{
    last_command="yc_resource-manager_folder_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_list-access-bindings()
{
    last_command="yc_resource-manager_folder_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_set-access-bindings()
{
    last_command="yc_resource-manager_folder_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_add-access-binding()
{
    last_command="yc_resource-manager_folder_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_remove-access-binding()
{
    last_command="yc_resource-manager_folder_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder_list-operations()
{
    last_command="yc_resource-manager_folder_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager_folder()
{
    last_command="yc_resource-manager_folder"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_resource-manager()
{
    last_command="yc_resource-manager"

    command_aliases=()

    commands=()
    commands+=("cloud")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clouds")
        aliashash["clouds"]="cloud"
    fi
    commands+=("folder")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("folders")
        aliashash["folders"]="folder"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_get()
{
    last_command="yc_compute_instance_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_list()
{
    last_command="yc_compute_instance_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_create()
{
    last_command="yc_compute_instance_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--ssh-key=")
    flags_with_completion+=("--ssh-key")
    flags_completion+=("__yc_handle_filename_extension_flag pub")
    flags+=("--public-ip")
    flags+=("--public-address=")
    flags+=("--create-disk=")
    flags+=("--attach-disk=")
    flags+=("--create-boot-disk=")
    flags+=("--use-boot-disk=")
    flags+=("--network-settings=")
    flags+=("--network-interface=")
    flags+=("--memory=")
    flags+=("--cores=")
    flags+=("--core-fraction=")
    flags+=("--gpus=")
    flags+=("--platform=")
    flags+=("--hostname=")
    flags+=("--preemptible")
    flags+=("--placement-group-id=")
    flags+=("--placement-group-name=")
    flags+=("--host-group-id=")
    flags+=("--host-group-name=")
    flags+=("--host-id=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--image-folder-id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_create-with-container()
{
    last_command="yc_compute_instance_create-with-container"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--ssh-key=")
    flags_with_completion+=("--ssh-key")
    flags_completion+=("__yc_handle_filename_extension_flag pub")
    flags+=("--public-ip")
    flags+=("--public-address=")
    flags+=("--create-disk=")
    flags+=("--attach-disk=")
    flags+=("--create-boot-disk=")
    flags+=("--use-boot-disk=")
    flags+=("--network-settings=")
    flags+=("--network-interface=")
    flags+=("--memory=")
    flags+=("--cores=")
    flags+=("--core-fraction=")
    flags+=("--gpus=")
    flags+=("--platform=")
    flags+=("--hostname=")
    flags+=("--preemptible")
    flags+=("--placement-group-id=")
    flags+=("--placement-group-name=")
    flags+=("--host-group-id=")
    flags+=("--host-group-name=")
    flags+=("--host-id=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--async")
    flags+=("--container-arg=")
    flags+=("--container-command=")
    flags+=("--container-env=")
    flags+=("--container-env-file=")
    flags_with_completion+=("--container-env-file")
    flags_completion+=("_filedir")
    flags+=("--container-image=")
    flags+=("--container-name=")
    flags+=("--container-privileged")
    flags+=("--container-stdin")
    flags+=("--container-tty")
    flags+=("--container-restart-policy=")
    flags+=("--container-volume-tmpfs=")
    flags+=("--container-volume-host-path=")
    flags+=("--docker-compose-file=")
    flags_with_completion+=("--docker-compose-file")
    flags_completion+=("_filedir")
    flags+=("--coi-spec-file=")
    flags_with_completion+=("--coi-spec-file")
    flags_completion+=("_filedir")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_update()
{
    last_command="yc_compute_instance_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--memory=")
    flags+=("--cores=")
    flags+=("--core-fraction=")
    flags+=("--gpus=")
    flags+=("--platform=")
    flags+=("--network-settings=")
    flags+=("--placement-group-id=")
    flags+=("--placement-group-name=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_update-container()
{
    last_command="yc_compute_instance_update-container"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--container-arg=")
    flags+=("--clear-container-args")
    flags+=("--container-command=")
    flags+=("--clear-container-command")
    flags+=("--container-env=")
    flags+=("--container-env-file=")
    flags_with_completion+=("--container-env-file")
    flags_completion+=("_filedir")
    flags+=("--remove-container-env=")
    flags+=("--container-image=")
    flags+=("--container-name=")
    flags+=("--container-privileged")
    flags+=("--container-stdin")
    flags+=("--container-tty")
    flags+=("--container-restart-policy=")
    flags+=("--container-volume-tmpfs=")
    flags+=("--container-volume-host-path=")
    flags+=("--remove-container-volume=")
    flags+=("--clear-container-volumes")
    flags+=("--docker-compose-file=")
    flags_with_completion+=("--docker-compose-file")
    flags_completion+=("_filedir")
    flags+=("--coi-spec-file=")
    flags_with_completion+=("--coi-spec-file")
    flags_completion+=("_filedir")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_add-metadata()
{
    last_command="yc_compute_instance_add-metadata"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_remove-metadata()
{
    last_command="yc_compute_instance_remove-metadata"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--all")
    flags+=("--keys=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_add-labels()
{
    last_command="yc_compute_instance_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_remove-labels()
{
    last_command="yc_compute_instance_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_delete()
{
    last_command="yc_compute_instance_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_get-serial-port-output()
{
    last_command="yc_compute_instance_get-serial-port-output"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--port=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_stop()
{
    last_command="yc_compute_instance_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_start()
{
    last_command="yc_compute_instance_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_restart()
{
    last_command="yc_compute_instance_restart"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_attach-disk()
{
    last_command="yc_compute_instance_attach-disk"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--disk-name=")
    flags+=("--disk-id=")
    flags+=("--device-name=")
    flags+=("--auto-delete")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_attach-new-disk()
{
    last_command="yc_compute_instance_attach-new-disk"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--disk-name=")
    flags+=("--type=")
    flags+=("--size=")
    flags+=("--block-size=")
    flags+=("--snapshot-name=")
    flags+=("--snapshot-id=")
    flags+=("--image-name=")
    flags+=("--image-id=")
    flags+=("--image-family=")
    flags+=("--image-folder-id=")
    flags+=("--disk-placement-group-id=")
    flags+=("--disk-placement-group-name=")
    flags+=("--device-name=")
    flags+=("--auto-delete")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_detach-disk()
{
    last_command="yc_compute_instance_detach-disk"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--disk-name=")
    flags+=("--disk-id=")
    flags+=("--device-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_update-network-interface()
{
    last_command="yc_compute_instance_update-network-interface"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--network-interface-index=")
    flags+=("--subnet-id=")
    flags+=("--subnet-name=")
    flags+=("--ipv4-address=")
    flags+=("--clear-ipv4-address")
    flags+=("--nat-ip-version=")
    flags+=("--nat-address=")
    flags+=("--security-group-id=")
    flags+=("--clear-security-groups")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_add-one-to-one-nat()
{
    last_command="yc_compute_instance_add-one-to-one-nat"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--network-interface-index=")
    flags+=("--internal-address=")
    flags+=("--nat-ip-version=")
    flags+=("--nat-address=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_remove-one-to-one-nat()
{
    last_command="yc_compute_instance_remove-one-to-one-nat"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--network-interface-index=")
    flags+=("--internal-address=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance_list-operations()
{
    last_command="yc_compute_instance_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance()
{
    last_command="yc_compute_instance"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("create-with-container")
    commands+=("update")
    commands+=("update-container")
    commands+=("add-metadata")
    commands+=("remove-metadata")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("get-serial-port-output")
    commands+=("stop")
    commands+=("start")
    commands+=("restart")
    commands+=("attach-disk")
    commands+=("attach-new-disk")
    commands+=("detach-disk")
    commands+=("update-network-interface")
    commands+=("add-one-to-one-nat")
    commands+=("remove-one-to-one-nat")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_get()
{
    last_command="yc_compute_disk_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_list()
{
    last_command="yc_compute_disk_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_create()
{
    last_command="yc_compute_disk_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--type=")
    flags+=("--size=")
    flags+=("--block-size=")
    flags+=("--source-image-name=")
    flags+=("--source-image-id=")
    flags+=("--source-image-family=")
    flags+=("--source-snapshot-name=")
    flags+=("--source-snapshot-id=")
    flags+=("--source-image-folder-id=")
    flags+=("--disk-placement-group-id=")
    flags+=("--disk-placement-group-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_update()
{
    last_command="yc_compute_disk_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--size=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_add-labels()
{
    last_command="yc_compute_disk_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_remove-labels()
{
    last_command="yc_compute_disk_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_resize()
{
    last_command="yc_compute_disk_resize"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--size=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_delete()
{
    last_command="yc_compute_disk_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk_list-operations()
{
    last_command="yc_compute_disk_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk()
{
    last_command="yc_compute_disk"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("resize")
    commands+=("delete")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-type_get()
{
    last_command="yc_compute_disk-type_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-type_list()
{
    last_command="yc_compute_disk-type_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-type()
{
    last_command="yc_compute_disk-type"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_get()
{
    last_command="yc_compute_image_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_list()
{
    last_command="yc_compute_image_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_get-latest-from-family()
{
    last_command="yc_compute_image_get-latest-from-family"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_create()
{
    last_command="yc_compute_image_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--family=")
    flags+=("--min-disk-size=")
    flags+=("--product-ids=")
    flags+=("--source-image-name=")
    flags+=("--source-image-id=")
    flags+=("--source-family=")
    flags+=("--source-snapshot-name=")
    flags+=("--source-snapshot-id=")
    flags+=("--source-disk-name=")
    flags+=("--source-disk-id=")
    flags+=("--source-uri=")
    flags+=("--source-image-folder-id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_update()
{
    last_command="yc_compute_image_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_add-labels()
{
    last_command="yc_compute_image_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_remove-labels()
{
    last_command="yc_compute_image_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_delete()
{
    last_command="yc_compute_image_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image_list-operations()
{
    last_command="yc_compute_image_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_image()
{
    last_command="yc_compute_image"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("get-latest-from-family")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot_get()
{
    last_command="yc_compute_snapshot_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot_list()
{
    last_command="yc_compute_snapshot_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot_create()
{
    last_command="yc_compute_snapshot_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--disk-id=")
    flags+=("--disk-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot_update()
{
    last_command="yc_compute_snapshot_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot_add-labels()
{
    last_command="yc_compute_snapshot_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot_remove-labels()
{
    last_command="yc_compute_snapshot_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot_delete()
{
    last_command="yc_compute_snapshot_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot_list-operations()
{
    last_command="yc_compute_snapshot_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_snapshot()
{
    last_command="yc_compute_snapshot"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_zone_get()
{
    last_command="yc_compute_zone_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_zone_list()
{
    last_command="yc_compute_zone_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_zone()
{
    last_command="yc_compute_zone"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_get()
{
    last_command="yc_compute_instance-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_list()
{
    last_command="yc_compute_instance-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_create()
{
    last_command="yc_compute_instance-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--file=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_update()
{
    last_command="yc_compute_instance-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--file=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_stop()
{
    last_command="yc_compute_instance-group_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_start()
{
    last_command="yc_compute_instance-group_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_add-metadata()
{
    last_command="yc_compute_instance-group_add-metadata"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_remove-metadata()
{
    last_command="yc_compute_instance-group_remove-metadata"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--all")
    flags+=("--keys=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_add-labels()
{
    last_command="yc_compute_instance-group_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_remove-labels()
{
    last_command="yc_compute_instance-group_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_delete()
{
    last_command="yc_compute_instance-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_list-instances()
{
    last_command="yc_compute_instance-group_list-instances"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_stop-instances()
{
    last_command="yc_compute_instance-group_stop-instances"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--instance-ids=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_delete-instances()
{
    last_command="yc_compute_instance-group_delete-instances"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--instance-ids=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_list-operations()
{
    last_command="yc_compute_instance-group_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_list-logs()
{
    last_command="yc_compute_instance-group_list-logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--filter=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_list-access-bindings()
{
    last_command="yc_compute_instance-group_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_set-access-bindings()
{
    last_command="yc_compute_instance-group_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_add-access-binding()
{
    last_command="yc_compute_instance-group_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group_remove-access-binding()
{
    last_command="yc_compute_instance-group_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_instance-group()
{
    last_command="yc_compute_instance-group"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("apply")
        aliashash["apply"]="update"
    fi
    commands+=("stop")
    commands+=("start")
    commands+=("add-metadata")
    commands+=("remove-metadata")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-instances")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("instances")
        aliashash["instances"]="list-instances"
    fi
    commands+=("stop-instances")
    commands+=("delete-instances")
    commands+=("list-operations")
    commands+=("list-logs")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("logs")
        aliashash["logs"]="list-logs"
    fi
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_get()
{
    last_command="yc_compute_placement-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_list()
{
    last_command="yc_compute_placement-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_create()
{
    last_command="yc_compute_placement-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--spread-strategy")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_update()
{
    last_command="yc_compute_placement-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_add-labels()
{
    last_command="yc_compute_placement-group_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_remove-labels()
{
    last_command="yc_compute_placement-group_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_delete()
{
    last_command="yc_compute_placement-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_list-instances()
{
    last_command="yc_compute_placement-group_list-instances"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group_list-operations()
{
    last_command="yc_compute_placement-group_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_placement-group()
{
    last_command="yc_compute_placement-group"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-instances")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("instances")
        aliashash["instances"]="list-instances"
    fi
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-type_get()
{
    last_command="yc_compute_host-type_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-type_list()
{
    last_command="yc_compute_host-type_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-type()
{
    last_command="yc_compute_host-type"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_get()
{
    last_command="yc_compute_host-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_list()
{
    last_command="yc_compute_host-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_create()
{
    last_command="yc_compute_host-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--type=")
    flags+=("--fixed-size=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_update()
{
    last_command="yc_compute_host-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--fixed-size=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_add-labels()
{
    last_command="yc_compute_host-group_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_remove-labels()
{
    last_command="yc_compute_host-group_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_delete()
{
    last_command="yc_compute_host-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_list-hosts()
{
    last_command="yc_compute_host-group_list-hosts"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_list-instances()
{
    last_command="yc_compute_host-group_list-instances"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group_list-operations()
{
    last_command="yc_compute_host-group_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_host-group()
{
    last_command="yc_compute_host-group"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-hosts")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("hosts")
        aliashash["hosts"]="list-hosts"
    fi
    commands+=("list-instances")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("instances")
        aliashash["instances"]="list-instances"
    fi
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_get()
{
    last_command="yc_compute_disk-placement-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_list()
{
    last_command="yc_compute_disk-placement-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_create()
{
    last_command="yc_compute_disk-placement-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_update()
{
    last_command="yc_compute_disk-placement-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_add-labels()
{
    last_command="yc_compute_disk-placement-group_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_remove-labels()
{
    last_command="yc_compute_disk-placement-group_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_delete()
{
    last_command="yc_compute_disk-placement-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_list-disks()
{
    last_command="yc_compute_disk-placement-group_list-disks"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group_list-operations()
{
    last_command="yc_compute_disk-placement-group_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_disk-placement-group()
{
    last_command="yc_compute_disk-placement-group"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-disks")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("disks")
        aliashash["disks"]="list-disks"
    fi
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute_connect-to-serial-port()
{
    last_command="yc_compute_connect-to-serial-port"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--instance-id=")
    flags+=("--instance-name=")
    flags+=("--user=")
    flags+=("--port=")
    flags+=("--ssh-key=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_compute()
{
    last_command="yc_compute"

    command_aliases=()

    commands=()
    commands+=("instance")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("instances")
        aliashash["instances"]="instance"
    fi
    commands+=("disk")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("disks")
        aliashash["disks"]="disk"
    fi
    commands+=("disk-type")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("disk-types")
        aliashash["disk-types"]="disk-type"
    fi
    commands+=("image")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("images")
        aliashash["images"]="image"
    fi
    commands+=("snapshot")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("snapshots")
        aliashash["snapshots"]="snapshot"
    fi
    commands+=("zone")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("zones")
        aliashash["zones"]="zone"
    fi
    commands+=("instance-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("ig")
        aliashash["ig"]="instance-group"
        command_aliases+=("instance-groups")
        aliashash["instance-groups"]="instance-group"
    fi
    commands+=("placement-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("pg")
        aliashash["pg"]="placement-group"
        command_aliases+=("placement-groups")
        aliashash["placement-groups"]="placement-group"
    fi
    commands+=("host-type")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("host-types")
        aliashash["host-types"]="host-type"
    fi
    commands+=("host-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("hg")
        aliashash["hg"]="host-group"
        command_aliases+=("host-groups")
        aliashash["host-groups"]="host-group"
    fi
    commands+=("disk-placement-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("disk-placement-groups")
        aliashash["disk-placement-groups"]="disk-placement-group"
    fi
    commands+=("connect-to-serial-port")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_get()
{
    last_command="yc_vpc_network_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_list()
{
    last_command="yc_vpc_network_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_create()
{
    last_command="yc_vpc_network_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_update()
{
    last_command="yc_vpc_network_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_add-labels()
{
    last_command="yc_vpc_network_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_remove-labels()
{
    last_command="yc_vpc_network_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_delete()
{
    last_command="yc_vpc_network_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_list-subnets()
{
    last_command="yc_vpc_network_list-subnets"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_list-operations()
{
    last_command="yc_vpc_network_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network_move()
{
    last_command="yc_vpc_network_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_network()
{
    last_command="yc_vpc_network"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-subnets")
    commands+=("list-operations")
    commands+=("move")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_get()
{
    last_command="yc_vpc_route-table_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_list()
{
    last_command="yc_vpc_route-table_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_create()
{
    last_command="yc_vpc_route-table_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--route=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_update()
{
    last_command="yc_vpc_route-table_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--clear-routes")
    flags+=("--route=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_add-labels()
{
    last_command="yc_vpc_route-table_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_remove-labels()
{
    last_command="yc_vpc_route-table_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_delete()
{
    last_command="yc_vpc_route-table_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_list-operations()
{
    last_command="yc_vpc_route-table_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table_move()
{
    last_command="yc_vpc_route-table_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_route-table()
{
    last_command="yc_vpc_route-table"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-operations")
    commands+=("move")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_get()
{
    last_command="yc_vpc_security-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_list()
{
    last_command="yc_vpc_security-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_create()
{
    last_command="yc_vpc_security-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--rule=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_update()
{
    last_command="yc_vpc_security-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--clear-rules")
    flags+=("--rule=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_add-labels()
{
    last_command="yc_vpc_security-group_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_remove-labels()
{
    last_command="yc_vpc_security-group_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_delete()
{
    last_command="yc_vpc_security-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_list-operations()
{
    last_command="yc_vpc_security-group_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_move()
{
    last_command="yc_vpc_security-group_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_update-rules()
{
    last_command="yc_vpc_security-group_update-rules"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--delete-rule-id=")
    flags+=("--add-rule=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group_update-rule-meta()
{
    last_command="yc_vpc_security-group_update-rule-meta"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--rule-id=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_security-group()
{
    last_command="yc_vpc_security-group"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-operations")
    commands+=("move")
    commands+=("update-rules")
    commands+=("update-rule-meta")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_get()
{
    last_command="yc_vpc_subnet_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_list()
{
    last_command="yc_vpc_subnet_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_create()
{
    last_command="yc_vpc_subnet_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--route-table-id=")
    flags+=("--route-table-name=")
    flags+=("--range=")
    flags+=("--domain-name=")
    flags+=("--domain-name-server=")
    flags+=("--ntp-server=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_update()
{
    last_command="yc_vpc_subnet_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--route-table-id=")
    flags+=("--route-table-name=")
    flags+=("--disassociate-route-table")
    flags+=("--domain-name=")
    flags+=("--domain-name-server=")
    flags+=("--ntp-server=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_add-labels()
{
    last_command="yc_vpc_subnet_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_remove-labels()
{
    last_command="yc_vpc_subnet_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_delete()
{
    last_command="yc_vpc_subnet_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_list-operations()
{
    last_command="yc_vpc_subnet_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_list-used-addresses()
{
    last_command="yc_vpc_subnet_list-used-addresses"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet_move()
{
    last_command="yc_vpc_subnet_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_subnet()
{
    last_command="yc_vpc_subnet"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-operations")
    commands+=("list-used-addresses")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("lua")
        aliashash["lua"]="list-used-addresses"
    fi
    commands+=("move")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_address_get()
{
    last_command="yc_vpc_address_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_address_list()
{
    last_command="yc_vpc_address_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_address_create()
{
    last_command="yc_vpc_address_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--async")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--external-ipv4=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_address_update()
{
    last_command="yc_vpc_address_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--reserved")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_address_delete()
{
    last_command="yc_vpc_address_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_address_list-operations()
{
    last_command="yc_vpc_address_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc_address()
{
    last_command="yc_vpc_address"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_vpc()
{
    last_command="yc_vpc"

    command_aliases=()

    commands=()
    commands+=("network")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("net")
        aliashash["net"]="network"
        command_aliases+=("networks")
        aliashash["networks"]="network"
    fi
    commands+=("route-table")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("route-tables")
        aliashash["route-tables"]="route-table"
    fi
    commands+=("security-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("security-groups")
        aliashash["security-groups"]="security-group"
        command_aliases+=("sg")
        aliashash["sg"]="security-group"
    fi
    commands+=("subnet")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("subnets")
        aliashash["subnets"]="subnet"
    fi
    commands+=("address")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("addr")
        aliashash["addr"]="address"
        command_aliases+=("addresses")
        aliashash["addresses"]="address"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_get()
{
    last_command="yc_dns_zone_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_list()
{
    last_command="yc_dns_zone_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_create()
{
    last_command="yc_dns_zone_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--public-visibility")
    flags+=("--private-visibility")
    flags+=("--network-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_update()
{
    last_command="yc_dns_zone_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--network-ids=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_delete()
{
    last_command="yc_dns_zone_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_list-records()
{
    last_command="yc_dns_zone_list-records"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_add-records()
{
    last_command="yc_dns_zone_add-records"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--record=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_delete-records()
{
    last_command="yc_dns_zone_delete-records"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--record=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_replace-records()
{
    last_command="yc_dns_zone_replace-records"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--record=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_add-labels()
{
    last_command="yc_dns_zone_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_remove-labels()
{
    last_command="yc_dns_zone_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_list-operations()
{
    last_command="yc_dns_zone_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_list-access-bindings()
{
    last_command="yc_dns_zone_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_set-access-bindings()
{
    last_command="yc_dns_zone_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_add-access-binding()
{
    last_command="yc_dns_zone_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone_remove-access-binding()
{
    last_command="yc_dns_zone_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns_zone()
{
    last_command="yc_dns_zone"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("list-records")
    commands+=("add-records")
    commands+=("delete-records")
    commands+=("replace-records")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("list-operations")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dns()
{
    last_command="yc_dns"

    command_aliases=()

    commands=()
    commands+=("zone")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("zones")
        aliashash["zones"]="zone"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_create-token()
{
    last_command="yc_managed-kubernetes_create-token"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_get-credentials()
{
    last_command="yc_managed-kubernetes_cluster_get-credentials"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--kubeconfig=")
    flags_with_completion+=("--kubeconfig")
    flags_completion+=("_filedir")
    flags+=("--context-name=")
    flags+=("--internal")
    flags+=("--external")
    flags+=("--force")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_get()
{
    last_command="yc_managed-kubernetes_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_list()
{
    last_command="yc_managed-kubernetes_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_create()
{
    last_command="yc_managed-kubernetes_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--cluster-ipv4-range=")
    flags+=("--service-ipv4-range=")
    flags+=("--cluster-ipv6-range=")
    flags+=("--service-ipv6-range=")
    flags+=("--dual-stack")
    flags+=("--node-ipv4-mask-size=")
    flags+=("--subnet-id=")
    flags+=("--subnet-name=")
    flags+=("--public-ip")
    flags+=("--regional")
    flags+=("--region=")
    flags+=("--master-location=")
    flags+=("--release-channel=")
    flags+=("--version=")
    flags+=("--enable-network-policy")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--node-service-account-id=")
    flags+=("--node-service-account-name=")
    flags+=("--auto-upgrade")
    flags+=("--anytime-maintenance-window")
    flags+=("--daily-maintenance-window=")
    flags+=("--weekly-maintenance-window=")
    flags+=("--kms-key-id=")
    flags+=("--kms-key-name=")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_update()
{
    last_command="yc_managed-kubernetes_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--default-gateway-v4-address=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--node-service-account-id=")
    flags+=("--node-service-account-name=")
    flags+=("--version=")
    flags+=("--latest-revision")
    flags+=("--auto-upgrade")
    flags+=("--security-group-ids=")
    flags+=("--anytime-maintenance-window")
    flags+=("--daily-maintenance-window=")
    flags+=("--weekly-maintenance-window=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_add-labels()
{
    last_command="yc_managed-kubernetes_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_remove-labels()
{
    last_command="yc_managed-kubernetes_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_delete()
{
    last_command="yc_managed-kubernetes_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_stop()
{
    last_command="yc_managed-kubernetes_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_start()
{
    last_command="yc_managed-kubernetes_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_list-node-groups()
{
    last_command="yc_managed-kubernetes_cluster_list-node-groups"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_list-operations()
{
    last_command="yc_managed-kubernetes_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster_list-nodes()
{
    last_command="yc_managed-kubernetes_cluster_list-nodes"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_cluster()
{
    last_command="yc_managed-kubernetes_cluster"

    command_aliases=()

    commands=()
    commands+=("get-credentials")
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("stop")
    commands+=("start")
    commands+=("list-node-groups")
    commands+=("list-operations")
    commands+=("list-nodes")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_get()
{
    last_command="yc_managed-kubernetes_node-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_list()
{
    last_command="yc_managed-kubernetes_node-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_create()
{
    last_command="yc_managed-kubernetes_node-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--platform=")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--public-ip")
    flags+=("--memory=")
    flags+=("--cores=")
    flags+=("--core-fraction=")
    flags+=("--gpus=")
    flags+=("--preemptible")
    flags+=("--disk-type=")
    flags+=("--disk-size=")
    flags+=("--fixed-size=")
    flags+=("--auto-scale=")
    flags+=("--location=")
    flags+=("--version=")
    flags+=("--allowed-unsafe-sysctls=")
    flags+=("--node-labels=")
    flags+=("--node-taints=")
    flags+=("--auto-upgrade")
    flags+=("--auto-repair")
    flags+=("--anytime-maintenance-window")
    flags+=("--daily-maintenance-window=")
    flags+=("--weekly-maintenance-window=")
    flags+=("--max-expansion=")
    flags+=("--max-unavailable=")
    flags+=("--network-interface=")
    flags+=("--placement-group=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_update()
{
    last_command="yc_managed-kubernetes_node-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--platform=")
    flags+=("--memory=")
    flags+=("--cores=")
    flags+=("--core-fraction=")
    flags+=("--gpus=")
    flags+=("--disk-type=")
    flags+=("--disk-size=")
    flags+=("--preemptible")
    flags+=("--fixed-size=")
    flags+=("--auto-scale=")
    flags+=("--version=")
    flags+=("--latest-revision")
    flags+=("--auto-upgrade")
    flags+=("--auto-repair")
    flags+=("--anytime-maintenance-window")
    flags+=("--daily-maintenance-window=")
    flags+=("--weekly-maintenance-window=")
    flags+=("--max-expansion=")
    flags+=("--max-unavailable=")
    flags+=("--network-interface=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_add-metadata()
{
    last_command="yc_managed-kubernetes_node-group_add-metadata"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_remove-metadata()
{
    last_command="yc_managed-kubernetes_node-group_remove-metadata"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--all")
    flags+=("--keys=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_add-labels()
{
    last_command="yc_managed-kubernetes_node-group_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_remove-labels()
{
    last_command="yc_managed-kubernetes_node-group_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_delete()
{
    last_command="yc_managed-kubernetes_node-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_list-operations()
{
    last_command="yc_managed-kubernetes_node-group_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group_list-nodes()
{
    last_command="yc_managed-kubernetes_node-group_list-nodes"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_node-group()
{
    last_command="yc_managed-kubernetes_node-group"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-metadata")
    commands+=("remove-metadata")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-operations")
    commands+=("list-nodes")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes_list-versions()
{
    last_command="yc_managed-kubernetes_list-versions"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kubernetes()
{
    last_command="yc_managed-kubernetes"

    command_aliases=()

    commands=()
    commands+=("create-token")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("token")
        aliashash["token"]="create-token"
    fi
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("node-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("node-groups")
        aliashash["node-groups"]="node-group"
    fi
    commands+=("list-versions")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_database_get()
{
    last_command="yc_ydb_database_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_database_list()
{
    last_command="yc_ydb_database_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_database_create()
{
    last_command="yc_ydb_database_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--resource-preset=")
    flags+=("--public-ip")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--subnet-name=")
    flags+=("--subnet-id=")
    flags+=("--storage=")
    flags+=("--fixed-size=")
    flags+=("--zone=")
    flags+=("--region=")
    flags+=("--location=")
    flags+=("--dedicated")
    flags+=("--serverless")
    flags+=("--backup=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_database_backup()
{
    last_command="yc_ydb_database_backup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--source=")
    flags+=("--exclude=")
    flags+=("--ttl=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_database_restore()
{
    last_command="yc_ydb_database_restore"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--backup-id=")
    flags+=("--target-path=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_database_update()
{
    last_command="yc_ydb_database_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--resource-preset=")
    flags+=("--public-ip")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--subnet-name=")
    flags+=("--subnet-id=")
    flags+=("--storage=")
    flags+=("--fixed-size=")
    flags+=("--zone=")
    flags+=("--region=")
    flags+=("--location=")
    flags+=("--dedicated")
    flags+=("--serverless")
    flags+=("--backup=")
    flags+=("--async")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_database_delete()
{
    last_command="yc_ydb_database_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_database()
{
    last_command="yc_ydb_database"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("backup")
    commands+=("restore")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_storage-type_get()
{
    last_command="yc_ydb_storage-type_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_storage-type_list()
{
    last_command="yc_ydb_storage-type_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_storage-type()
{
    last_command="yc_ydb_storage-type"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_resource-preset_get()
{
    last_command="yc_ydb_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_resource-preset_list()
{
    last_command="yc_ydb_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_resource-preset()
{
    last_command="yc_ydb_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_backup_get()
{
    last_command="yc_ydb_backup_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_backup_list()
{
    last_command="yc_ydb_backup_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_backup_delete()
{
    last_command="yc_ydb_backup_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb_backup()
{
    last_command="yc_ydb_backup"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_ydb()
{
    last_command="yc_ydb"

    command_aliases=()

    commands=()
    commands+=("database")
    commands+=("storage-type")
    commands+=("resource-preset")
    commands+=("backup")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_get()
{
    last_command="yc_kms_symmetric-key_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_list()
{
    last_command="yc_kms_symmetric-key_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_list-versions()
{
    last_command="yc_kms_symmetric-key_list-versions"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_create()
{
    last_command="yc_kms_symmetric-key_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--default-algorithm=")
    flags+=("--rotation-period=")
    flags+=("--deletion-protection")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_update()
{
    last_command="yc_kms_symmetric-key_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--status=")
    flags+=("--labels=")
    flags+=("--default-algorithm=")
    flags+=("--rotation-period=")
    flags+=("--deletion-protection")
    flags+=("--no-deletion-protection")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_delete()
{
    last_command="yc_kms_symmetric-key_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_set-primary-version()
{
    last_command="yc_kms_symmetric-key_set-primary-version"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--version-id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_schedule-version-destruction()
{
    last_command="yc_kms_symmetric-key_schedule-version-destruction"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--version-id=")
    flags+=("--pending-period=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_cancel-version-destruction()
{
    last_command="yc_kms_symmetric-key_cancel-version-destruction"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--version-id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_rotate()
{
    last_command="yc_kms_symmetric-key_rotate"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_list-operations()
{
    last_command="yc_kms_symmetric-key_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_list-access-bindings()
{
    last_command="yc_kms_symmetric-key_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_set-access-bindings()
{
    last_command="yc_kms_symmetric-key_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_add-access-binding()
{
    last_command="yc_kms_symmetric-key_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key_remove-access-binding()
{
    last_command="yc_kms_symmetric-key_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-key()
{
    last_command="yc_kms_symmetric-key"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("list-versions")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("list-version")
        aliashash["list-version"]="list-versions"
    fi
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("set-primary-version")
    commands+=("schedule-version-destruction")
    commands+=("cancel-version-destruction")
    commands+=("rotate")
    commands+=("list-operations")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-crypto_encrypt()
{
    last_command="yc_kms_symmetric-crypto_encrypt"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--version-id=")
    flags+=("--aad-context-file=")
    flags+=("--plaintext-file=")
    flags+=("--ciphertext-file=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-crypto_decrypt()
{
    last_command="yc_kms_symmetric-crypto_decrypt"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--aad-context-file=")
    flags+=("--ciphertext-file=")
    flags+=("--plaintext-file=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-crypto_generate-data-key()
{
    last_command="yc_kms_symmetric-crypto_generate-data-key"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--version-id=")
    flags+=("--aad-context-file=")
    flags+=("--data-key-spec=")
    flags+=("--skip-plaintext")
    flags+=("--data-key-plaintext-file=")
    flags+=("--data-key-ciphertext-file=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-crypto_re-encrypt()
{
    last_command="yc_kms_symmetric-crypto_re-encrypt"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--version-id=")
    flags+=("--aad-context-file=")
    flags+=("--source-key-id=")
    flags+=("--source-aad-context-file=")
    flags+=("--source-ciphertext-file=")
    flags+=("--ciphertext-file=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms_symmetric-crypto()
{
    last_command="yc_kms_symmetric-crypto"

    command_aliases=()

    commands=()
    commands+=("encrypt")
    commands+=("decrypt")
    commands+=("generate-data-key")
    commands+=("re-encrypt")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_kms()
{
    last_command="yc_kms"

    command_aliases=()

    commands=()
    commands+=("symmetric-key")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("symmetric-keys")
        aliashash["symmetric-keys"]="symmetric-key"
    fi
    commands+=("symmetric-crypto")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_get()
{
    last_command="yc_certificate-manager_certificate_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_content()
{
    last_command="yc_certificate-manager_certificate_content"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--chain=")
    flags+=("--key=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_list()
{
    last_command="yc_certificate-manager_certificate_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_create()
{
    last_command="yc_certificate-manager_certificate_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--chain=")
    flags+=("--key=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_request()
{
    last_command="yc_certificate-manager_certificate_request"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--domains=")
    flags+=("--challenge=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_update()
{
    last_command="yc_certificate-manager_certificate_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--chain=")
    flags+=("--key=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_delete()
{
    last_command="yc_certificate-manager_certificate_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_list-operations()
{
    last_command="yc_certificate-manager_certificate_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_list-access-bindings()
{
    last_command="yc_certificate-manager_certificate_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_set-access-bindings()
{
    last_command="yc_certificate-manager_certificate_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_add-access-binding()
{
    last_command="yc_certificate-manager_certificate_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate_remove-access-binding()
{
    last_command="yc_certificate-manager_certificate_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager_certificate()
{
    last_command="yc_certificate-manager_certificate"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("content")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("download")
        aliashash["download"]="content"
        command_aliases+=("get-content")
        aliashash["get-content"]="content"
    fi
    commands+=("list")
    commands+=("create")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("import")
        aliashash["import"]="create"
    fi
    commands+=("request")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("new")
        aliashash["new"]="request"
        command_aliases+=("request-new")
        aliashash["request-new"]="request"
    fi
    commands+=("update")
    commands+=("delete")
    commands+=("list-operations")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_certificate-manager()
{
    last_command="yc_certificate-manager"

    command_aliases=()

    commands=()
    commands+=("certificate")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("certificates")
        aliashash["certificates"]="certificate"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_get()
{
    last_command="yc_managed-clickhouse_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_list()
{
    last_command="yc_managed-clickhouse_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_create()
{
    last_command="yc_managed-clickhouse_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--shard-name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--user=")
    flags+=("--database=")
    flags+=("--clickhouse-resource-preset=")
    flags+=("--clickhouse-disk-size=")
    flags+=("--clickhouse-disk-type=")
    flags+=("--zookeeper-resource-preset=")
    flags+=("--zookeeper-disk-size=")
    flags+=("--zookeeper-disk-type=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--metrika-access")
    flags+=("--serverless-access")
    flags+=("--version=")
    flags+=("--backup-window-start=")
    flags+=("--cloud-storage")
    flags+=("--enable-sql-database-management")
    flags+=("--enable-sql-user-management")
    flags+=("--labels=")
    flags+=("--service-account=")
    flags+=("--security-group-ids=")
    flags+=("--admin-password=")
    flags+=("--read-admin-password")
    flags+=("--generate-admin-password")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_restore()
{
    last_command="yc_managed-clickhouse_cluster_restore"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backup-id=")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--clickhouse-resource-preset=")
    flags+=("--clickhouse-disk-size=")
    flags+=("--clickhouse-disk-type=")
    flags+=("--zookeeper-resource-preset=")
    flags+=("--zookeeper-disk-size=")
    flags+=("--zookeeper-disk-type=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--metrika-access")
    flags+=("--serverless-access")
    flags+=("--version=")
    flags+=("--backup-window-start=")
    flags+=("--cloud-storage")
    flags+=("--enable-sql-database-management")
    flags+=("--enable-sql-user-management")
    flags+=("--labels=")
    flags+=("--service-account=")
    flags+=("--security-group-ids=")
    flags+=("--admin-password=")
    flags+=("--read-admin-password")
    flags+=("--generate-admin-password")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_reschedule-maintenance()
{
    last_command="yc_managed-clickhouse_cluster_reschedule-maintenance"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--reschedule-type=")
    flags+=("--delayed-until=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_update()
{
    last_command="yc_managed-clickhouse_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--service-account=")
    flags+=("--security-group-ids=")
    flags+=("--clickhouse-resource-preset=")
    flags+=("--clickhouse-disk-size=")
    flags+=("--clickhouse-disk-type=")
    flags+=("--zookeeper-resource-preset=")
    flags+=("--zookeeper-disk-size=")
    flags+=("--zookeeper-disk-type=")
    flags+=("--version=")
    flags+=("--backup-window-start=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--metrika-access")
    flags+=("--serverless-access")
    flags+=("--enable-sql-database-management")
    flags+=("--enable-sql-user-management")
    flags+=("--admin-password=")
    flags+=("--read-admin-password")
    flags+=("--generate-admin-password")
    flags+=("--maintenance-window=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_update-config()
{
    last_command="yc_managed-clickhouse_cluster_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_add-labels()
{
    last_command="yc_managed-clickhouse_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_remove-labels()
{
    last_command="yc_managed-clickhouse_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_delete()
{
    last_command="yc_managed-clickhouse_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_add-zookeeper()
{
    last_command="yc_managed-clickhouse_cluster_add-zookeeper"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_backup()
{
    last_command="yc_managed-clickhouse_cluster_backup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_list-backups()
{
    last_command="yc_managed-clickhouse_cluster_list-backups"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_list-logs()
{
    last_command="yc_managed-clickhouse_cluster_list-logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--service-type=")
    flags+=("--columns=")
    flags+=("--filter=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_list-operations()
{
    last_command="yc_managed-clickhouse_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_clear-compression()
{
    last_command="yc_managed-clickhouse_cluster_clear-compression"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_set-compression()
{
    last_command="yc_managed-clickhouse_cluster_set-compression"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--with=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_add-external-dictionary()
{
    last_command="yc_managed-clickhouse_cluster_add-external-dictionary"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--dict-name=")
    flags+=("--structure-id=")
    flags+=("--structure-key=")
    flags+=("--structure-attribute=")
    flags+=("--fixed-lifetime=")
    flags+=("--lifetime-range=")
    flags+=("--layout-type=")
    flags+=("--layout-size-in-cells=")
    flags+=("--http-source-url=")
    flags+=("--http-source-format=")
    flags+=("--http-source=")
    flags+=("--mongodb-source=")
    flags+=("--clickhouse-source=")
    flags+=("--mysql-source=")
    flags+=("--mysql-replica=")
    flags+=("--mysql-invalidate-query=")
    flags+=("--postgresql-source=")
    flags+=("--postgresql-source-hosts=")
    flags+=("--postgresql-invalidate-query=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_remove-external-dictionary()
{
    last_command="yc_managed-clickhouse_cluster_remove-external-dictionary"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--dict-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_add-graphite-rollup()
{
    last_command="yc_managed-clickhouse_cluster_add-graphite-rollup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--rollup-file-name=")
    flags_with_completion+=("--rollup-file-name")
    flags_completion+=("_filedir")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_remove-graphite-rollup()
{
    last_command="yc_managed-clickhouse_cluster_remove-graphite-rollup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--rollup-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_move()
{
    last_command="yc_managed-clickhouse_cluster_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_start()
{
    last_command="yc_managed-clickhouse_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster_stop()
{
    last_command="yc_managed-clickhouse_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_cluster()
{
    last_command="yc_managed-clickhouse_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("restore")
    commands+=("reschedule-maintenance")
    commands+=("update")
    commands+=("update-config")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("add-zookeeper")
    commands+=("backup")
    commands+=("list-backups")
    commands+=("list-logs")
    commands+=("list-operations")
    commands+=("clear-compression")
    commands+=("set-compression")
    commands+=("add-external-dictionary")
    commands+=("remove-external-dictionary")
    commands+=("add-graphite-rollup")
    commands+=("remove-graphite-rollup")
    commands+=("move")
    commands+=("start")
    commands+=("stop")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_hosts_list()
{
    last_command="yc_managed-clickhouse_hosts_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_hosts_add()
{
    last_command="yc_managed-clickhouse_hosts_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--copy-schema")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_hosts_delete()
{
    last_command="yc_managed-clickhouse_hosts_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_hosts()
{
    last_command="yc_managed-clickhouse_hosts"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shards_get()
{
    last_command="yc_managed-clickhouse_shards_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shards_list()
{
    last_command="yc_managed-clickhouse_shards_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shards_add()
{
    last_command="yc_managed-clickhouse_shards_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--host=")
    flags+=("--clickhouse-resource-preset=")
    flags+=("--clickhouse-disk-size=")
    flags+=("--clickhouse-disk-type=")
    flags+=("--weight=")
    flags+=("--copy-schema")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shards_update()
{
    last_command="yc_managed-clickhouse_shards_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--clickhouse-resource-preset=")
    flags+=("--clickhouse-disk-size=")
    flags+=("--clickhouse-disk-type=")
    flags+=("--weight=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shards_update-config()
{
    last_command="yc_managed-clickhouse_shards_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shards_delete()
{
    last_command="yc_managed-clickhouse_shards_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shards()
{
    last_command="yc_managed-clickhouse_shards"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("add")
    commands+=("update")
    commands+=("update-config")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shard-groups_get()
{
    last_command="yc_managed-clickhouse_shard-groups_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shard-groups_list()
{
    last_command="yc_managed-clickhouse_shard-groups_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shard-groups_create()
{
    last_command="yc_managed-clickhouse_shard-groups_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--shards=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shard-groups_update()
{
    last_command="yc_managed-clickhouse_shard-groups_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--shards=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shard-groups_delete()
{
    last_command="yc_managed-clickhouse_shard-groups_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_shard-groups()
{
    last_command="yc_managed-clickhouse_shard-groups"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_database_get()
{
    last_command="yc_managed-clickhouse_database_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_database_list()
{
    last_command="yc_managed-clickhouse_database_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_database_create()
{
    last_command="yc_managed-clickhouse_database_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_database_delete()
{
    last_command="yc_managed-clickhouse_database_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_database()
{
    last_command="yc_managed-clickhouse_database"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_get()
{
    last_command="yc_managed-clickhouse_user_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_list()
{
    last_command="yc_managed-clickhouse_user_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_create()
{
    last_command="yc_managed-clickhouse_user_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--permissions=")
    flags+=("--settings=")
    flags+=("--quota=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_update()
{
    last_command="yc_managed-clickhouse_user_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--permissions=")
    flags+=("--settings=")
    flags+=("--quota=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_delete()
{
    last_command="yc_managed-clickhouse_user_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_grant-permission()
{
    last_command="yc_managed-clickhouse_user_grant-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_revoke-permission()
{
    last_command="yc_managed-clickhouse_user_revoke-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_add-quota()
{
    last_command="yc_managed-clickhouse_user_add-quota"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--interval=")
    flags+=("--queries=")
    flags+=("--errors=")
    flags+=("--result-rows=")
    flags+=("--read-rows=")
    flags+=("--execution-time=")
    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_update-quota()
{
    last_command="yc_managed-clickhouse_user_update-quota"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--interval=")
    flags+=("--queries=")
    flags+=("--errors=")
    flags+=("--result-rows=")
    flags+=("--read-rows=")
    flags+=("--execution-time=")
    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user_remove-quota()
{
    last_command="yc_managed-clickhouse_user_remove-quota"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--interval=")
    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_user()
{
    last_command="yc_managed-clickhouse_user"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("grant-permission")
    commands+=("revoke-permission")
    commands+=("add-quota")
    commands+=("update-quota")
    commands+=("remove-quota")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_backup_get()
{
    last_command="yc_managed-clickhouse_backup_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_backup_list()
{
    last_command="yc_managed-clickhouse_backup_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_backup()
{
    last_command="yc_managed-clickhouse_backup"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_resource-preset_get()
{
    last_command="yc_managed-clickhouse_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_resource-preset_list()
{
    last_command="yc_managed-clickhouse_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_resource-preset()
{
    last_command="yc_managed-clickhouse_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_ml-model_get()
{
    last_command="yc_managed-clickhouse_ml-model_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_ml-model_list()
{
    last_command="yc_managed-clickhouse_ml-model_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_ml-model_create()
{
    last_command="yc_managed-clickhouse_ml-model_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--type=")
    flags+=("--uri=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_ml-model_update()
{
    last_command="yc_managed-clickhouse_ml-model_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--uri=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_ml-model_delete()
{
    last_command="yc_managed-clickhouse_ml-model_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_ml-model()
{
    last_command="yc_managed-clickhouse_ml-model"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_format-schema_get()
{
    last_command="yc_managed-clickhouse_format-schema_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_format-schema_list()
{
    last_command="yc_managed-clickhouse_format-schema_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_format-schema_create()
{
    last_command="yc_managed-clickhouse_format-schema_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--type=")
    flags+=("--uri=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_format-schema_update()
{
    last_command="yc_managed-clickhouse_format-schema_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--uri=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_format-schema_delete()
{
    last_command="yc_managed-clickhouse_format-schema_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_format-schema()
{
    last_command="yc_managed-clickhouse_format-schema"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_version_list()
{
    last_command="yc_managed-clickhouse_version_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse_version()
{
    last_command="yc_managed-clickhouse_version"

    command_aliases=()

    commands=()
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-clickhouse()
{
    last_command="yc_managed-clickhouse"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("hosts")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("host")
        aliashash["host"]="hosts"
    fi
    commands+=("shards")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("shard")
        aliashash["shard"]="shards"
    fi
    commands+=("shard-groups")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("shard-group")
        aliashash["shard-group"]="shard-groups"
    fi
    commands+=("database")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("databases")
        aliashash["databases"]="database"
    fi
    commands+=("user")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("users")
        aliashash["users"]="user"
    fi
    commands+=("backup")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("backups")
        aliashash["backups"]="backup"
    fi
    commands+=("resource-preset")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource-presets")
        aliashash["resource-presets"]="resource-preset"
    fi
    commands+=("ml-model")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("ml-models")
        aliashash["ml-models"]="ml-model"
    fi
    commands+=("format-schema")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("format-schemas")
        aliashash["format-schemas"]="format-schema"
    fi
    commands+=("version")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("versions")
        aliashash["versions"]="version"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_get()
{
    last_command="yc_managed-mongodb_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_list()
{
    last_command="yc_managed-mongodb_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_create()
{
    last_command="yc_managed-mongodb_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--user=")
    flags+=("--database=")
    flags+=("--mongodb-version=")
    flags+=("--backup-window-start=")
    flags+=("--backup-retain-period-days=")
    flags+=("--datalens-access")
    flags+=("--mongod-resource-preset=")
    flags+=("--mongod-disk-size=")
    flags+=("--mongod-disk-type=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_restore()
{
    last_command="yc_managed-mongodb_cluster_restore"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backup-id=")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--recovery-target-timestamp=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--mongodb-version=")
    flags+=("--backup-window-start=")
    flags+=("--backup-retain-period-days=")
    flags+=("--datalens-access")
    flags+=("--mongod-resource-preset=")
    flags+=("--mongod-disk-size=")
    flags+=("--mongod-disk-type=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_reschedule-maintenance()
{
    last_command="yc_managed-mongodb_cluster_reschedule-maintenance"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--reschedule-type=")
    flags+=("--delayed-until=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_update()
{
    last_command="yc_managed-mongodb_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--maintenance-window=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--feature-compatibility-version=")
    flags+=("--security-group-ids=")
    flags+=("--mongodb-version=")
    flags+=("--backup-window-start=")
    flags+=("--backup-retain-period-days=")
    flags+=("--datalens-access")
    flags+=("--mongod-resource-preset=")
    flags+=("--mongod-disk-size=")
    flags+=("--mongod-disk-type=")
    flags+=("--mongocfg-resource-preset=")
    flags+=("--mongocfg-disk-size=")
    flags+=("--mongocfg-disk-type=")
    flags+=("--mongos-resource-preset=")
    flags+=("--mongos-disk-size=")
    flags+=("--mongos-disk-type=")
    flags+=("--mongoinfra-resource-preset=")
    flags+=("--mongoinfra-disk-size=")
    flags+=("--mongoinfra-disk-type=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_update-config()
{
    last_command="yc_managed-mongodb_cluster_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_add-labels()
{
    last_command="yc_managed-mongodb_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_remove-labels()
{
    last_command="yc_managed-mongodb_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_delete()
{
    last_command="yc_managed-mongodb_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_backup()
{
    last_command="yc_managed-mongodb_cluster_backup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_list-backups()
{
    last_command="yc_managed-mongodb_cluster_list-backups"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_list-logs()
{
    last_command="yc_managed-mongodb_cluster_list-logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--service-type=")
    flags+=("--columns=")
    flags+=("--filter=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_list-operations()
{
    last_command="yc_managed-mongodb_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_move()
{
    last_command="yc_managed-mongodb_cluster_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_start()
{
    last_command="yc_managed-mongodb_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_stop()
{
    last_command="yc_managed-mongodb_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster_enable-sharding()
{
    last_command="yc_managed-mongodb_cluster_enable-sharding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--mongocfg=")
    flags+=("--mongos=")
    flags+=("--mongoinfra=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_cluster()
{
    last_command="yc_managed-mongodb_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("restore")
    commands+=("reschedule-maintenance")
    commands+=("update")
    commands+=("update-config")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("backup")
    commands+=("list-backups")
    commands+=("list-logs")
    commands+=("list-operations")
    commands+=("move")
    commands+=("start")
    commands+=("stop")
    commands+=("enable-sharding")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_hosts_list()
{
    last_command="yc_managed-mongodb_hosts_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_hosts_add()
{
    last_command="yc_managed-mongodb_hosts_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_hosts_delete()
{
    last_command="yc_managed-mongodb_hosts_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_hosts_resetup()
{
    last_command="yc_managed-mongodb_hosts_resetup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_hosts()
{
    last_command="yc_managed-mongodb_hosts"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")
    commands+=("resetup")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_database_get()
{
    last_command="yc_managed-mongodb_database_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_database_list()
{
    last_command="yc_managed-mongodb_database_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_database_create()
{
    last_command="yc_managed-mongodb_database_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_database_delete()
{
    last_command="yc_managed-mongodb_database_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_database()
{
    last_command="yc_managed-mongodb_database"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_user_get()
{
    last_command="yc_managed-mongodb_user_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_user_list()
{
    last_command="yc_managed-mongodb_user_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_user_create()
{
    last_command="yc_managed-mongodb_user_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--permission=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_user_update()
{
    last_command="yc_managed-mongodb_user_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--permission=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_user_delete()
{
    last_command="yc_managed-mongodb_user_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_user_grant-permission()
{
    last_command="yc_managed-mongodb_user_grant-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--role=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_user_revoke-permission()
{
    last_command="yc_managed-mongodb_user_revoke-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_user()
{
    last_command="yc_managed-mongodb_user"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("grant-permission")
    commands+=("revoke-permission")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_backup_get()
{
    last_command="yc_managed-mongodb_backup_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_backup_list()
{
    last_command="yc_managed-mongodb_backup_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_backup()
{
    last_command="yc_managed-mongodb_backup"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_resource-preset_get()
{
    last_command="yc_managed-mongodb_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_resource-preset_list()
{
    last_command="yc_managed-mongodb_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_resource-preset()
{
    last_command="yc_managed-mongodb_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_shards_get()
{
    last_command="yc_managed-mongodb_shards_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_shards_list()
{
    last_command="yc_managed-mongodb_shards_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_shards_add()
{
    last_command="yc_managed-mongodb_shards_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_shards_delete()
{
    last_command="yc_managed-mongodb_shards_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb_shards()
{
    last_command="yc_managed-mongodb_shards"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mongodb()
{
    last_command="yc_managed-mongodb"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("hosts")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("host")
        aliashash["host"]="hosts"
    fi
    commands+=("database")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("databases")
        aliashash["databases"]="database"
    fi
    commands+=("user")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("users")
        aliashash["users"]="user"
    fi
    commands+=("backup")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("backups")
        aliashash["backups"]="backup"
    fi
    commands+=("resource-preset")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource-presets")
        aliashash["resource-presets"]="resource-preset"
    fi
    commands+=("shards")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("shard")
        aliashash["shard"]="shards"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_get()
{
    last_command="yc_managed-mysql_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_list()
{
    last_command="yc_managed-mysql_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_create()
{
    last_command="yc_managed-mysql_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--user=")
    flags+=("--database=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--mysql-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--backup-window-start=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_restore()
{
    last_command="yc_managed-mysql_cluster_restore"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backup-id=")
    flags+=("--time=")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--mysql-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--backup-window-start=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_start-failover()
{
    last_command="yc_managed-mysql_cluster_start-failover"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_reschedule-maintenance()
{
    last_command="yc_managed-mysql_cluster_reschedule-maintenance"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--reschedule-type=")
    flags+=("--delayed-until=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_update()
{
    last_command="yc_managed-mysql_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--maintenance-window=")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--mysql-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--backup-window-start=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_update-config()
{
    last_command="yc_managed-mysql_cluster_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_add-labels()
{
    last_command="yc_managed-mysql_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_remove-labels()
{
    last_command="yc_managed-mysql_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_delete()
{
    last_command="yc_managed-mysql_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_backup()
{
    last_command="yc_managed-mysql_cluster_backup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_list-backups()
{
    last_command="yc_managed-mysql_cluster_list-backups"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_list-logs()
{
    last_command="yc_managed-mysql_cluster_list-logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--service-type=")
    flags+=("--columns=")
    flags+=("--filter=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_list-operations()
{
    last_command="yc_managed-mysql_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_move()
{
    last_command="yc_managed-mysql_cluster_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_start()
{
    last_command="yc_managed-mysql_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster_stop()
{
    last_command="yc_managed-mysql_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_cluster()
{
    last_command="yc_managed-mysql_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("restore")
    commands+=("start-failover")
    commands+=("reschedule-maintenance")
    commands+=("update")
    commands+=("update-config")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("backup")
    commands+=("list-backups")
    commands+=("list-logs")
    commands+=("list-operations")
    commands+=("move")
    commands+=("start")
    commands+=("stop")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_hosts_list()
{
    last_command="yc_managed-mysql_hosts_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_hosts_add()
{
    last_command="yc_managed-mysql_hosts_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_hosts_delete()
{
    last_command="yc_managed-mysql_hosts_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_hosts()
{
    last_command="yc_managed-mysql_hosts"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_database_get()
{
    last_command="yc_managed-mysql_database_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_database_list()
{
    last_command="yc_managed-mysql_database_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_database_create()
{
    last_command="yc_managed-mysql_database_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_database_delete()
{
    last_command="yc_managed-mysql_database_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_database()
{
    last_command="yc_managed-mysql_database"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_user_get()
{
    last_command="yc_managed-mysql_user_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_user_list()
{
    last_command="yc_managed-mysql_user_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_user_create()
{
    last_command="yc_managed-mysql_user_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--permissions=")
    flags+=("--global-permissions=")
    flags+=("--authentication-plugin=")
    flags+=("--max-questions-per-hour=")
    flags+=("--max-updates-per-hour=")
    flags+=("--max-connections-per-hour=")
    flags+=("--max-user-connections=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_user_update()
{
    last_command="yc_managed-mysql_user_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--permissions=")
    flags+=("--global-permissions=")
    flags+=("--authentication-plugin=")
    flags+=("--max-questions-per-hour=")
    flags+=("--max-updates-per-hour=")
    flags+=("--max-connections-per-hour=")
    flags+=("--max-user-connections=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_user_delete()
{
    last_command="yc_managed-mysql_user_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_user_grant-permission()
{
    last_command="yc_managed-mysql_user_grant-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--permissions=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_user_revoke-permission()
{
    last_command="yc_managed-mysql_user_revoke-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--permissions=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_user()
{
    last_command="yc_managed-mysql_user"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("grant-permission")
    commands+=("revoke-permission")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_backup_get()
{
    last_command="yc_managed-mysql_backup_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_backup_list()
{
    last_command="yc_managed-mysql_backup_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_backup()
{
    last_command="yc_managed-mysql_backup"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_resource-preset_get()
{
    last_command="yc_managed-mysql_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_resource-preset_list()
{
    last_command="yc_managed-mysql_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql_resource-preset()
{
    last_command="yc_managed-mysql_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-mysql()
{
    last_command="yc_managed-mysql"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("hosts")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("host")
        aliashash["host"]="hosts"
    fi
    commands+=("database")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("databases")
        aliashash["databases"]="database"
    fi
    commands+=("user")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("users")
        aliashash["users"]="user"
    fi
    commands+=("backup")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("backups")
        aliashash["backups"]="backup"
    fi
    commands+=("resource-preset")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource-presets")
        aliashash["resource-presets"]="resource-preset"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_get()
{
    last_command="yc_managed-sqlserver_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_list()
{
    last_command="yc_managed-sqlserver_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_create()
{
    last_command="yc_managed-sqlserver_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--user=")
    flags+=("--database=")
    flags+=("--sqlserver-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--backup-window-start=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--confirm-payment")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_restore()
{
    last_command="yc_managed-sqlserver_cluster_restore"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backup-id=")
    flags+=("--time=")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--sqlserver-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--backup-window-start=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--confirm-payment")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_update()
{
    last_command="yc_managed-sqlserver_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--backup-window-start=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_update-config()
{
    last_command="yc_managed-sqlserver_cluster_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_add-labels()
{
    last_command="yc_managed-sqlserver_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_remove-labels()
{
    last_command="yc_managed-sqlserver_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_delete()
{
    last_command="yc_managed-sqlserver_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_backup()
{
    last_command="yc_managed-sqlserver_cluster_backup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_list-backups()
{
    last_command="yc_managed-sqlserver_cluster_list-backups"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_list-operations()
{
    last_command="yc_managed-sqlserver_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_start()
{
    last_command="yc_managed-sqlserver_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster_stop()
{
    last_command="yc_managed-sqlserver_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_cluster()
{
    last_command="yc_managed-sqlserver_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("restore")
    commands+=("update")
    commands+=("update-config")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("backup")
    commands+=("list-backups")
    commands+=("list-operations")
    commands+=("start")
    commands+=("stop")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_host_list()
{
    last_command="yc_managed-sqlserver_host_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_host()
{
    last_command="yc_managed-sqlserver_host"

    command_aliases=()

    commands=()
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_database_get()
{
    last_command="yc_managed-sqlserver_database_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_database_list()
{
    last_command="yc_managed-sqlserver_database_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_database_create()
{
    last_command="yc_managed-sqlserver_database_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_database_delete()
{
    last_command="yc_managed-sqlserver_database_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_database()
{
    last_command="yc_managed-sqlserver_database"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_user_get()
{
    last_command="yc_managed-sqlserver_user_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_user_list()
{
    last_command="yc_managed-sqlserver_user_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_user_create()
{
    last_command="yc_managed-sqlserver_user_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--databases=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_user_update()
{
    last_command="yc_managed-sqlserver_user_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--databases=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_user_delete()
{
    last_command="yc_managed-sqlserver_user_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_user_grant-permission()
{
    last_command="yc_managed-sqlserver_user_grant-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--roles=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_user_revoke-permission()
{
    last_command="yc_managed-sqlserver_user_revoke-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--roles=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_user()
{
    last_command="yc_managed-sqlserver_user"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("grant-permission")
    commands+=("revoke-permission")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_backup_get()
{
    last_command="yc_managed-sqlserver_backup_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_backup_list()
{
    last_command="yc_managed-sqlserver_backup_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_backup()
{
    last_command="yc_managed-sqlserver_backup"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_resource-preset_get()
{
    last_command="yc_managed-sqlserver_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_resource-preset_list()
{
    last_command="yc_managed-sqlserver_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver_resource-preset()
{
    last_command="yc_managed-sqlserver_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-sqlserver()
{
    last_command="yc_managed-sqlserver"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("host")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("hosts")
        aliashash["hosts"]="host"
    fi
    commands+=("database")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("databases")
        aliashash["databases"]="database"
    fi
    commands+=("user")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("users")
        aliashash["users"]="user"
    fi
    commands+=("backup")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("backups")
        aliashash["backups"]="backup"
    fi
    commands+=("resource-preset")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource-presets")
        aliashash["resource-presets"]="resource-preset"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_get()
{
    last_command="yc_managed-postgresql_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_list()
{
    last_command="yc_managed-postgresql_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_create()
{
    last_command="yc_managed-postgresql_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--user=")
    flags+=("--database=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--postgresql-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--backup-window-start=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_restore()
{
    last_command="yc_managed-postgresql_cluster_restore"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backup-id=")
    flags+=("--time=")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--postgresql-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--backup-window-start=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_start-failover()
{
    last_command="yc_managed-postgresql_cluster_start-failover"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_reschedule-maintenance()
{
    last_command="yc_managed-postgresql_cluster_reschedule-maintenance"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--reschedule-type=")
    flags+=("--delayed-until=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_update()
{
    last_command="yc_managed-postgresql_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--connection-pooling-mode=")
    flags+=("--connection-pool-discard")
    flags+=("--postgresql-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--autofailover")
    flags+=("--backup-window-start=")
    flags+=("--datalens-access")
    flags+=("--websql-access")
    flags+=("--maintenance-window=")
    flags+=("--performance-diagnostics=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_update-config()
{
    last_command="yc_managed-postgresql_cluster_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_add-labels()
{
    last_command="yc_managed-postgresql_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_remove-labels()
{
    last_command="yc_managed-postgresql_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_delete()
{
    last_command="yc_managed-postgresql_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_backup()
{
    last_command="yc_managed-postgresql_cluster_backup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_list-backups()
{
    last_command="yc_managed-postgresql_cluster_list-backups"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_list-logs()
{
    last_command="yc_managed-postgresql_cluster_list-logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--service-type=")
    flags+=("--columns=")
    flags+=("--filter=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_list-operations()
{
    last_command="yc_managed-postgresql_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_move()
{
    last_command="yc_managed-postgresql_cluster_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_start()
{
    last_command="yc_managed-postgresql_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster_stop()
{
    last_command="yc_managed-postgresql_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_cluster()
{
    last_command="yc_managed-postgresql_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("restore")
    commands+=("start-failover")
    commands+=("reschedule-maintenance")
    commands+=("update")
    commands+=("update-config")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("backup")
    commands+=("list-backups")
    commands+=("list-logs")
    commands+=("list-operations")
    commands+=("move")
    commands+=("start")
    commands+=("stop")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_hosts_list()
{
    last_command="yc_managed-postgresql_hosts_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_hosts_add()
{
    last_command="yc_managed-postgresql_hosts_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_hosts_update()
{
    last_command="yc_managed-postgresql_hosts_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--replication-source=")
    flags+=("--priority=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_hosts_delete()
{
    last_command="yc_managed-postgresql_hosts_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_hosts_update-config()
{
    last_command="yc_managed-postgresql_hosts_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_hosts()
{
    last_command="yc_managed-postgresql_hosts"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("update")
    commands+=("delete")
    commands+=("update-config")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_database_get()
{
    last_command="yc_managed-postgresql_database_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_database_list()
{
    last_command="yc_managed-postgresql_database_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_database_create()
{
    last_command="yc_managed-postgresql_database_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--owner=")
    flags+=("--lc-collate=")
    flags+=("--lc-type=")
    flags+=("--extensions=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_database_update()
{
    last_command="yc_managed-postgresql_database_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--extensions=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_database_delete()
{
    last_command="yc_managed-postgresql_database_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_database()
{
    last_command="yc_managed-postgresql_database"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_user_get()
{
    last_command="yc_managed-postgresql_user_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_user_list()
{
    last_command="yc_managed-postgresql_user_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_user_create()
{
    last_command="yc_managed-postgresql_user_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--conn-limit=")
    flags+=("--permissions=")
    flags+=("--login")
    flags+=("--grants=")
    flags+=("--lock-timeout=")
    flags+=("--log-min-duration-statement=")
    flags+=("--temp-file-limit=")
    flags+=("--default-transaction-isolation=")
    flags+=("--synchronous-commit=")
    flags+=("--log-statement=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_user_update()
{
    last_command="yc_managed-postgresql_user_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--conn-limit=")
    flags+=("--permissions=")
    flags+=("--login")
    flags+=("--grants=")
    flags+=("--lock-timeout=")
    flags+=("--log-min-duration-statement=")
    flags+=("--temp-file-limit=")
    flags+=("--default-transaction-isolation=")
    flags+=("--synchronous-commit=")
    flags+=("--log-statement=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_user_delete()
{
    last_command="yc_managed-postgresql_user_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_user_grant-permission()
{
    last_command="yc_managed-postgresql_user_grant-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_user_revoke-permission()
{
    last_command="yc_managed-postgresql_user_revoke-permission"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--database=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_user()
{
    last_command="yc_managed-postgresql_user"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("grant-permission")
    commands+=("revoke-permission")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_backup_get()
{
    last_command="yc_managed-postgresql_backup_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_backup_list()
{
    last_command="yc_managed-postgresql_backup_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_backup()
{
    last_command="yc_managed-postgresql_backup"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_resource-preset_get()
{
    last_command="yc_managed-postgresql_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_resource-preset_list()
{
    last_command="yc_managed-postgresql_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql_resource-preset()
{
    last_command="yc_managed-postgresql_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-postgresql()
{
    last_command="yc_managed-postgresql"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("hosts")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("host")
        aliashash["host"]="hosts"
    fi
    commands+=("database")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("databases")
        aliashash["databases"]="database"
    fi
    commands+=("user")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("users")
        aliashash["users"]="user"
    fi
    commands+=("backup")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("backups")
        aliashash["backups"]="backup"
    fi
    commands+=("resource-preset")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource-presets")
        aliashash["resource-presets"]="resource-preset"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_get()
{
    last_command="yc_managed-redis_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_list()
{
    last_command="yc_managed-redis_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_create()
{
    last_command="yc_managed-redis_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--redis-version=")
    flags+=("--password=")
    flags+=("--max-memory-policy=")
    flags+=("--timeout=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type-id=")
    flags+=("--backup-window-start=")
    flags+=("--datalens-access")
    flags+=("--labels=")
    flags+=("--sharded")
    flags+=("--enable-tls")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_restore()
{
    last_command="yc_managed-redis_cluster_restore"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backup-id=")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--redis-version=")
    flags+=("--password=")
    flags+=("--max-memory-policy=")
    flags+=("--timeout=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type-id=")
    flags+=("--backup-window-start=")
    flags+=("--datalens-access")
    flags+=("--labels=")
    flags+=("--enable-tls")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_start-failover()
{
    last_command="yc_managed-redis_cluster_start-failover"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_reschedule-maintenance()
{
    last_command="yc_managed-redis_cluster_reschedule-maintenance"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--reschedule-type=")
    flags+=("--delayed-until=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_update()
{
    last_command="yc_managed-redis_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--redis-version=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--backup-window-start=")
    flags+=("--datalens-access")
    flags+=("--maintenance-window=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_update-config()
{
    last_command="yc_managed-redis_cluster_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_add-labels()
{
    last_command="yc_managed-redis_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_remove-labels()
{
    last_command="yc_managed-redis_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_delete()
{
    last_command="yc_managed-redis_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_rebalance()
{
    last_command="yc_managed-redis_cluster_rebalance"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_backup()
{
    last_command="yc_managed-redis_cluster_backup"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_list-backups()
{
    last_command="yc_managed-redis_cluster_list-backups"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_list-logs()
{
    last_command="yc_managed-redis_cluster_list-logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--service-type=")
    flags+=("--columns=")
    flags+=("--filter=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_list-operations()
{
    last_command="yc_managed-redis_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_move()
{
    last_command="yc_managed-redis_cluster_move"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--destination-folder-id=")
    flags+=("--destination-folder-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_start()
{
    last_command="yc_managed-redis_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster_stop()
{
    last_command="yc_managed-redis_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_cluster()
{
    last_command="yc_managed-redis_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("restore")
    commands+=("start-failover")
    commands+=("reschedule-maintenance")
    commands+=("update")
    commands+=("update-config")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("rebalance")
    commands+=("backup")
    commands+=("list-backups")
    commands+=("list-logs")
    commands+=("list-operations")
    commands+=("move")
    commands+=("start")
    commands+=("stop")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_hosts_list()
{
    last_command="yc_managed-redis_hosts_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_hosts_add()
{
    last_command="yc_managed-redis_hosts_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_hosts_delete()
{
    last_command="yc_managed-redis_hosts_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_hosts()
{
    last_command="yc_managed-redis_hosts"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_backup_get()
{
    last_command="yc_managed-redis_backup_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_backup_list()
{
    last_command="yc_managed-redis_backup_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_backup()
{
    last_command="yc_managed-redis_backup"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_resource-preset_get()
{
    last_command="yc_managed-redis_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_resource-preset_list()
{
    last_command="yc_managed-redis_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_resource-preset()
{
    last_command="yc_managed-redis_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_shards_get()
{
    last_command="yc_managed-redis_shards_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_shards_list()
{
    last_command="yc_managed-redis_shards_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_shards_add()
{
    last_command="yc_managed-redis_shards_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_shards_delete()
{
    last_command="yc_managed-redis_shards_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis_shards()
{
    last_command="yc_managed-redis_shards"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-redis()
{
    last_command="yc_managed-redis"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("hosts")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("host")
        aliashash["host"]="hosts"
    fi
    commands+=("backup")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("backups")
        aliashash["backups"]="backup"
    fi
    commands+=("resource-preset")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource-presets")
        aliashash["resource-presets"]="resource-preset"
    fi
    commands+=("shards")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("shard")
        aliashash["shard"]="shards"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_get()
{
    last_command="yc_managed-elasticsearch_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--full")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_list()
{
    last_command="yc_managed-elasticsearch_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_create()
{
    last_command="yc_managed-elasticsearch_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--host=")
    flags+=("--user=")
    flags+=("--datanode-resource-preset=")
    flags+=("--datanode-disk-size=")
    flags+=("--datanode-disk-type=")
    flags+=("--masternode-resource-preset=")
    flags+=("--masternode-disk-size=")
    flags+=("--masternode-disk-type=")
    flags+=("--plugins=")
    flags+=("--version=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_delete()
{
    last_command="yc_managed-elasticsearch_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_list-logs()
{
    last_command="yc_managed-elasticsearch_cluster_list-logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--columns=")
    flags+=("--filter=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_list-operations()
{
    last_command="yc_managed-elasticsearch_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_start()
{
    last_command="yc_managed-elasticsearch_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_stop()
{
    last_command="yc_managed-elasticsearch_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_update()
{
    last_command="yc_managed-elasticsearch_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--datanode-resource-preset=")
    flags+=("--datanode-disk-size=")
    flags+=("--datanode-disk-type=")
    flags+=("--masternode-resource-preset=")
    flags+=("--masternode-disk-size=")
    flags+=("--masternode-disk-type=")
    flags+=("--plugins=")
    flags+=("--version=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_update-config()
{
    last_command="yc_managed-elasticsearch_cluster_update-config"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--set=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_add-labels()
{
    last_command="yc_managed-elasticsearch_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster_remove-labels()
{
    last_command="yc_managed-elasticsearch_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_cluster()
{
    last_command="yc_managed-elasticsearch_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("delete")
    commands+=("list-logs")
    commands+=("list-operations")
    commands+=("start")
    commands+=("stop")
    commands+=("update")
    commands+=("update-config")
    commands+=("add-labels")
    commands+=("remove-labels")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_hosts_list()
{
    last_command="yc_managed-elasticsearch_hosts_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_hosts_add()
{
    last_command="yc_managed-elasticsearch_hosts_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--host=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_hosts_delete()
{
    last_command="yc_managed-elasticsearch_hosts_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_hosts()
{
    last_command="yc_managed-elasticsearch_hosts"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_resource-preset_get()
{
    last_command="yc_managed-elasticsearch_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_resource-preset_list()
{
    last_command="yc_managed-elasticsearch_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_resource-preset()
{
    last_command="yc_managed-elasticsearch_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_user_get()
{
    last_command="yc_managed-elasticsearch_user_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_user_list()
{
    last_command="yc_managed-elasticsearch_user_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_user_create()
{
    last_command="yc_managed-elasticsearch_user_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_user_update()
{
    last_command="yc_managed-elasticsearch_user_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--password=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_user_delete()
{
    last_command="yc_managed-elasticsearch_user_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch_user()
{
    last_command="yc_managed-elasticsearch_user"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-elasticsearch()
{
    last_command="yc_managed-elasticsearch"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("hosts")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("host")
        aliashash["host"]="hosts"
    fi
    commands+=("resource-preset")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource-presets")
        aliashash["resource-presets"]="resource-preset"
    fi
    commands+=("user")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("users")
        aliashash["users"]="user"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_get()
{
    last_command="yc_managed-kafka_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_list()
{
    last_command="yc_managed-kafka_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_delete()
{
    last_command="yc_managed-kafka_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_list-operations()
{
    last_command="yc_managed-kafka_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_list-hosts()
{
    last_command="yc_managed-kafka_cluster_list-hosts"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_start()
{
    last_command="yc_managed-kafka_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_stop()
{
    last_command="yc_managed-kafka_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_create()
{
    last_command="yc_managed-kafka_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--labels=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--version=")
    flags+=("--zone-ids=")
    flags+=("--brokers-count=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--disk-type=")
    flags+=("--zookeeper-resource-preset=")
    flags+=("--zookeeper-disk-size=")
    flags+=("--zookeeper-disk-type=")
    flags+=("--assign-public-ip")
    flags+=("--unmanaged-topics")
    flags+=("--subnet-ids=")
    flags+=("--security-group-ids=")
    flags+=("--compression-type=")
    flags+=("--log-flush-interval-messages=")
    flags+=("--log-flush-interval-ms=")
    flags+=("--log-flush-scheduler-interval-ms=")
    flags+=("--log-retention-bytes=")
    flags+=("--log-retention-hours=")
    flags+=("--log-retention-minutes=")
    flags+=("--log-retention-ms=")
    flags+=("--log-segment-bytes=")
    flags+=("--log-preallocate")
    flags+=("--auto-create-topics-enable")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster_update()
{
    last_command="yc_managed-kafka_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--security-group-ids=")
    flags+=("--zone-ids=")
    flags+=("--brokers-count=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--zookeeper-resource-preset=")
    flags+=("--zookeeper-disk-size=")
    flags+=("--unmanaged-topics")
    flags+=("--compression-type=")
    flags+=("--log-flush-interval-messages=")
    flags+=("--log-flush-interval-ms=")
    flags+=("--log-flush-scheduler-interval-ms=")
    flags+=("--log-retention-bytes=")
    flags+=("--log-retention-hours=")
    flags+=("--log-retention-minutes=")
    flags+=("--log-retention-ms=")
    flags+=("--log-segment-bytes=")
    flags+=("--log-preallocate")
    flags+=("--auto-create-topics-enable")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_cluster()
{
    last_command="yc_managed-kafka_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("delete")
    commands+=("list-operations")
    commands+=("list-hosts")
    commands+=("start")
    commands+=("stop")
    commands+=("create")
    commands+=("update")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_topic_get()
{
    last_command="yc_managed-kafka_topic_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_topic_list()
{
    last_command="yc_managed-kafka_topic_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_topic_create()
{
    last_command="yc_managed-kafka_topic_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--partitions=")
    flags+=("--replication-factor=")
    flags+=("--cleanup-policy=")
    flags+=("--compression-type=")
    flags+=("--delete-retention-ms=")
    flags+=("--file-delete-delay-ms=")
    flags+=("--flush-messages=")
    flags+=("--flush-ms=")
    flags+=("--min-compaction-lag-ms=")
    flags+=("--retention-bytes=")
    flags+=("--retention-ms=")
    flags+=("--max-message-bytes=")
    flags+=("--min-insync-replicas=")
    flags+=("--segment-bytes=")
    flags+=("--preallocate")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_topic_update()
{
    last_command="yc_managed-kafka_topic_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--partitions=")
    flags+=("--replication-factor=")
    flags+=("--cleanup-policy=")
    flags+=("--compression-type=")
    flags+=("--delete-retention-ms=")
    flags+=("--file-delete-delay-ms=")
    flags+=("--flush-messages=")
    flags+=("--flush-ms=")
    flags+=("--min-compaction-lag-ms=")
    flags+=("--retention-bytes=")
    flags+=("--retention-ms=")
    flags+=("--max-message-bytes=")
    flags+=("--min-insync-replicas=")
    flags+=("--segment-bytes=")
    flags+=("--preallocate")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_topic_delete()
{
    last_command="yc_managed-kafka_topic_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_topic()
{
    last_command="yc_managed-kafka_topic"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_user_get()
{
    last_command="yc_managed-kafka_user_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_user_list()
{
    last_command="yc_managed-kafka_user_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_user_create()
{
    last_command="yc_managed-kafka_user_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--password=")
    flags+=("--permission=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_user_update()
{
    last_command="yc_managed-kafka_user_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--password=")
    flags+=("--permission=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_user_delete()
{
    last_command="yc_managed-kafka_user_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka_user()
{
    last_command="yc_managed-kafka_user"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_managed-kafka()
{
    last_command="yc_managed-kafka"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("topic")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("topics")
        aliashash["topics"]="topic"
    fi
    commands+=("user")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("users")
        aliashash["users"]="user"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_get()
{
    last_command="yc_container_registry_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_list()
{
    last_command="yc_container_registry_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_create()
{
    last_command="yc_container_registry_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--labels=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_update()
{
    last_command="yc_container_registry_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_add-labels()
{
    last_command="yc_container_registry_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_remove-labels()
{
    last_command="yc_container_registry_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_delete()
{
    last_command="yc_container_registry_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_list-access-bindings()
{
    last_command="yc_container_registry_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_set-access-bindings()
{
    last_command="yc_container_registry_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_add-access-binding()
{
    last_command="yc_container_registry_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_remove-access-binding()
{
    last_command="yc_container_registry_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_list-ip-permissions()
{
    last_command="yc_container_registry_list-ip-permissions"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_set-ip-permissions()
{
    last_command="yc_container_registry_set-ip-permissions"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--pull=")
    flags+=("--push=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_add-ip-permissions()
{
    last_command="yc_container_registry_add-ip-permissions"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--pull=")
    flags+=("--push=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_remove-ip-permissions()
{
    last_command="yc_container_registry_remove-ip-permissions"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--pull=")
    flags+=("--push=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry_configure-docker()
{
    last_command="yc_container_registry_configure-docker"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--disable")
    flags+=("--skip-version-check")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_registry()
{
    last_command="yc_container_registry"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")
    commands+=("list-ip-permissions")
    commands+=("set-ip-permissions")
    commands+=("add-ip-permissions")
    commands+=("remove-ip-permissions")
    commands+=("configure-docker")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_get()
{
    last_command="yc_container_repository_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_list()
{
    last_command="yc_container_repository_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_list-access-bindings()
{
    last_command="yc_container_repository_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_set-access-bindings()
{
    last_command="yc_container_repository_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_add-access-binding()
{
    last_command="yc_container_repository_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_remove-access-binding()
{
    last_command="yc_container_repository_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_get()
{
    last_command="yc_container_repository_lifecycle-policy_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_list()
{
    last_command="yc_container_repository_lifecycle-policy_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--repository-id=")
    flags+=("--repository-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_create()
{
    last_command="yc_container_repository_lifecycle-policy_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--repository-id=")
    flags+=("--repository-name=")
    flags+=("--active")
    flags+=("--description=")
    flags+=("--rules=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_update()
{
    last_command="yc_container_repository_lifecycle-policy_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--async")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--new-name=")
    flags+=("--new-description=")
    flags+=("--activate")
    flags+=("--disable")
    flags+=("--new-rules=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_delete()
{
    last_command="yc_container_repository_lifecycle-policy_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_dry-run()
{
    last_command="yc_container_repository_lifecycle-policy_dry-run"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_list-dry-run-results()
{
    last_command="yc_container_repository_lifecycle-policy_list-dry-run-results"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_get-dry-run-result()
{
    last_command="yc_container_repository_lifecycle-policy_get-dry-run-result"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy_list-dry-run-affected-images()
{
    last_command="yc_container_repository_lifecycle-policy_list-dry-run-affected-images"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository_lifecycle-policy()
{
    last_command="yc_container_repository_lifecycle-policy"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("dry-run")
    commands+=("list-dry-run-results")
    commands+=("get-dry-run-result")
    commands+=("list-dry-run-affected-images")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_repository()
{
    last_command="yc_container_repository"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")
    commands+=("lifecycle-policy")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("lifecycle-policies")
        aliashash["lifecycle-policies"]="lifecycle-policy"
        command_aliases+=("lp")
        aliashash["lp"]="lifecycle-policy"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image_get()
{
    last_command="yc_container_image_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image_list()
{
    last_command="yc_container_image_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--repository-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image_delete()
{
    last_command="yc_container_image_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image_scan()
{
    last_command="yc_container_image_scan"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image_get-scan-result()
{
    last_command="yc_container_image_get-scan-result"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image_get-last-scan-result()
{
    last_command="yc_container_image_get-last-scan-result"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--image-id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image_list-scan-results()
{
    last_command="yc_container_image_list-scan-results"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--image-id=")
    flags+=("--repository-id=")
    flags+=("--repository-name=")
    flags+=("--limit=")
    flags+=("--filter=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image_list-vulnerabilities()
{
    last_command="yc_container_image_list-vulnerabilities"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--scan-result-id=")
    flags+=("--limit=")
    flags+=("--filter=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_image()
{
    last_command="yc_container_image"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("delete")
    commands+=("scan")
    commands+=("get-scan-result")
    commands+=("get-last-scan-result")
    commands+=("list-scan-results")
    commands+=("list-vulnerabilities")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_get-credentials()
{
    last_command="yc_container_cluster_get-credentials"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--kubeconfig=")
    flags_with_completion+=("--kubeconfig")
    flags_completion+=("_filedir")
    flags+=("--context-name=")
    flags+=("--internal")
    flags+=("--external")
    flags+=("--force")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_get()
{
    last_command="yc_container_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_list()
{
    last_command="yc_container_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_create()
{
    last_command="yc_container_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--labels=")
    flags+=("--zone=")
    flags+=("--cluster-ipv4-range=")
    flags+=("--service-ipv4-range=")
    flags+=("--cluster-ipv6-range=")
    flags+=("--service-ipv6-range=")
    flags+=("--dual-stack")
    flags+=("--node-ipv4-mask-size=")
    flags+=("--subnet-id=")
    flags+=("--subnet-name=")
    flags+=("--public-ip")
    flags+=("--regional")
    flags+=("--region=")
    flags+=("--master-location=")
    flags+=("--release-channel=")
    flags+=("--version=")
    flags+=("--enable-network-policy")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--node-service-account-id=")
    flags+=("--node-service-account-name=")
    flags+=("--auto-upgrade")
    flags+=("--anytime-maintenance-window")
    flags+=("--daily-maintenance-window=")
    flags+=("--weekly-maintenance-window=")
    flags+=("--kms-key-id=")
    flags+=("--kms-key-name=")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_update()
{
    last_command="yc_container_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--default-gateway-v4-address=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--node-service-account-id=")
    flags+=("--node-service-account-name=")
    flags+=("--version=")
    flags+=("--latest-revision")
    flags+=("--auto-upgrade")
    flags+=("--security-group-ids=")
    flags+=("--anytime-maintenance-window")
    flags+=("--daily-maintenance-window=")
    flags+=("--weekly-maintenance-window=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_add-labels()
{
    last_command="yc_container_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_remove-labels()
{
    last_command="yc_container_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_delete()
{
    last_command="yc_container_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_stop()
{
    last_command="yc_container_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_start()
{
    last_command="yc_container_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_list-node-groups()
{
    last_command="yc_container_cluster_list-node-groups"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_list-operations()
{
    last_command="yc_container_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster_list-nodes()
{
    last_command="yc_container_cluster_list-nodes"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_cluster()
{
    last_command="yc_container_cluster"

    command_aliases=()

    commands=()
    commands+=("get-credentials")
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("stop")
    commands+=("start")
    commands+=("list-node-groups")
    commands+=("list-operations")
    commands+=("list-nodes")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_get()
{
    last_command="yc_container_node-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_list()
{
    last_command="yc_container_node-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_create()
{
    last_command="yc_container_node-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--platform=")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--public-ip")
    flags+=("--memory=")
    flags+=("--cores=")
    flags+=("--core-fraction=")
    flags+=("--gpus=")
    flags+=("--preemptible")
    flags+=("--disk-type=")
    flags+=("--disk-size=")
    flags+=("--fixed-size=")
    flags+=("--auto-scale=")
    flags+=("--location=")
    flags+=("--version=")
    flags+=("--allowed-unsafe-sysctls=")
    flags+=("--node-labels=")
    flags+=("--node-taints=")
    flags+=("--auto-upgrade")
    flags+=("--auto-repair")
    flags+=("--anytime-maintenance-window")
    flags+=("--daily-maintenance-window=")
    flags+=("--weekly-maintenance-window=")
    flags+=("--max-expansion=")
    flags+=("--max-unavailable=")
    flags+=("--network-interface=")
    flags+=("--placement-group=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_update()
{
    last_command="yc_container_node-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--platform=")
    flags+=("--memory=")
    flags+=("--cores=")
    flags+=("--core-fraction=")
    flags+=("--gpus=")
    flags+=("--disk-type=")
    flags+=("--disk-size=")
    flags+=("--preemptible")
    flags+=("--fixed-size=")
    flags+=("--auto-scale=")
    flags+=("--version=")
    flags+=("--latest-revision")
    flags+=("--auto-upgrade")
    flags+=("--auto-repair")
    flags+=("--anytime-maintenance-window")
    flags+=("--daily-maintenance-window=")
    flags+=("--weekly-maintenance-window=")
    flags+=("--max-expansion=")
    flags+=("--max-unavailable=")
    flags+=("--network-interface=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_add-metadata()
{
    last_command="yc_container_node-group_add-metadata"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--metadata=")
    flags+=("--metadata-from-file=")
    flags_with_completion+=("--metadata-from-file")
    flags_completion+=("_filedir")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_remove-metadata()
{
    last_command="yc_container_node-group_remove-metadata"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--all")
    flags+=("--keys=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_add-labels()
{
    last_command="yc_container_node-group_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_remove-labels()
{
    last_command="yc_container_node-group_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_delete()
{
    last_command="yc_container_node-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_list-operations()
{
    last_command="yc_container_node-group_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group_list-nodes()
{
    last_command="yc_container_node-group_list-nodes"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container_node-group()
{
    last_command="yc_container_node-group"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-metadata")
    commands+=("remove-metadata")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-operations")
    commands+=("list-nodes")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_container()
{
    last_command="yc_container"

    command_aliases=()

    commands=()
    commands+=("registry")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("reg")
        aliashash["reg"]="registry"
        command_aliases+=("registries")
        aliashash["registries"]="registry"
    fi
    commands+=("repository")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("repo")
        aliashash["repo"]="repository"
        command_aliases+=("repositories")
        aliashash["repositories"]="repository"
    fi
    commands+=("image")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("images")
        aliashash["images"]="image"
        command_aliases+=("img")
        aliashash["img"]="image"
    fi
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("node-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("node-groups")
        aliashash["node-groups"]="node-group"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_get()
{
    last_command="yc_load-balancer_network-load-balancer_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_list()
{
    last_command="yc_load-balancer_network-load-balancer_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_create()
{
    last_command="yc_load-balancer_network-load-balancer_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--region-id=")
    flags+=("--type=")
    flags+=("--target-group=")
    flags+=("--listener=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_update()
{
    last_command="yc_load-balancer_network-load-balancer_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--target-group=")
    flags+=("--listener=")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_delete()
{
    last_command="yc_load-balancer_network-load-balancer_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_start()
{
    last_command="yc_load-balancer_network-load-balancer_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_stop()
{
    last_command="yc_load-balancer_network-load-balancer_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_attach-target-group()
{
    last_command="yc_load-balancer_network-load-balancer_attach-target-group"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--target-group=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_detach-target-group()
{
    last_command="yc_load-balancer_network-load-balancer_detach-target-group"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--target-group-id=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_target-states()
{
    last_command="yc_load-balancer_network-load-balancer_target-states"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--target-group-id=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_add-listener()
{
    last_command="yc_load-balancer_network-load-balancer_add-listener"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--listener=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_remove-listener()
{
    last_command="yc_load-balancer_network-load-balancer_remove-listener"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--listener-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer_list-operations()
{
    last_command="yc_load-balancer_network-load-balancer_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_network-load-balancer()
{
    last_command="yc_load-balancer_network-load-balancer"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("start")
    commands+=("stop")
    commands+=("attach-target-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("attach")
        aliashash["attach"]="attach-target-group"
    fi
    commands+=("detach-target-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("detach")
        aliashash["detach"]="detach-target-group"
    fi
    commands+=("target-states")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("states")
        aliashash["states"]="target-states"
    fi
    commands+=("add-listener")
    commands+=("remove-listener")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group_get()
{
    last_command="yc_load-balancer_target-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group_list()
{
    last_command="yc_load-balancer_target-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group_create()
{
    last_command="yc_load-balancer_target-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--region-id=")
    flags+=("--target=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group_update()
{
    last_command="yc_load-balancer_target-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--target=")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group_delete()
{
    last_command="yc_load-balancer_target-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group_add-targets()
{
    last_command="yc_load-balancer_target-group_add-targets"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--target=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group_remove-targets()
{
    last_command="yc_load-balancer_target-group_remove-targets"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--target=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group_list-operations()
{
    last_command="yc_load-balancer_target-group_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer_target-group()
{
    last_command="yc_load-balancer_target-group"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("add-targets")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("add")
        aliashash["add"]="add-targets"
    fi
    commands+=("remove-targets")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_load-balancer()
{
    last_command="yc_load-balancer"

    command_aliases=()

    commands=()
    commands+=("network-load-balancer")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("nlb")
        aliashash["nlb"]="network-load-balancer"
    fi
    commands+=("target-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("tg")
        aliashash["tg"]="target-group"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_operation_get()
{
    last_command="yc_operation_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_operation_wait()
{
    last_command="yc_operation_wait"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_operation()
{
    last_command="yc_operation"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("wait")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_list()
{
    last_command="yc_config_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_get()
{
    last_command="yc_config_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_set()
{
    last_command="yc_config_set"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_unset()
{
    last_command="yc_config_unset"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_profile_get()
{
    last_command="yc_config_profile_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_profile_list()
{
    last_command="yc_config_profile_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_profile_create()
{
    last_command="yc_config_profile_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_profile_delete()
{
    last_command="yc_config_profile_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_profile_activate()
{
    last_command="yc_config_profile_activate"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config_profile()
{
    last_command="yc_config_profile"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("delete")
    commands+=("activate")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_config()
{
    last_command="yc_config"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("get")
    commands+=("set")
    commands+=("unset")
    commands+=("profile")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("profiles")
        aliashash["profiles"]="profile"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_init()
{
    last_command="yc_init"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--federation-id=")
    flags+=("--federation-endpoint=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_version()
{
    last_command="yc_version"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--semantic")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_components_update()
{
    last_command="yc_components_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_components_restore()
{
    last_command="yc_components_restore"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_components()
{
    last_command="yc_components"

    command_aliases=()

    commands=()
    commands+=("update")
    commands+=("restore")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_get()
{
    last_command="yc_serverless_function_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_list()
{
    last_command="yc_serverless_function_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_create()
{
    last_command="yc_serverless_function_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_update()
{
    last_command="yc_serverless_function_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_add-labels()
{
    last_command="yc_serverless_function_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_remove-labels()
{
    last_command="yc_serverless_function_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_delete()
{
    last_command="yc_serverless_function_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_tag-history()
{
    last_command="yc_serverless_function_tag-history"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--tag=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_list-operations()
{
    last_command="yc_serverless_function_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_logs()
{
    last_command="yc_serverless_function_logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--tag=")
    flags+=("--limit=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("-f")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_invoke()
{
    last_command="yc_serverless_function_invoke"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--tag=")
    flags+=("--data=")
    two_word_flags+=("-d")
    flags+=("--data-file=")
    flags+=("--data-stdin")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_list-access-bindings()
{
    last_command="yc_serverless_function_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_set-access-bindings()
{
    last_command="yc_serverless_function_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_add-access-binding()
{
    last_command="yc_serverless_function_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_remove-access-binding()
{
    last_command="yc_serverless_function_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_allow-unauthenticated-invoke()
{
    last_command="yc_serverless_function_allow-unauthenticated-invoke"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_deny-unauthenticated-invoke()
{
    last_command="yc_serverless_function_deny-unauthenticated-invoke"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_version_list()
{
    last_command="yc_serverless_function_version_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--function-id=")
    flags+=("--function-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_version_get()
{
    last_command="yc_serverless_function_version_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_version_create()
{
    last_command="yc_serverless_function_version_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--function-id=")
    flags+=("--function-name=")
    flags+=("--async")
    flags+=("--runtime=")
    flags+=("--entrypoint=")
    flags+=("--memory=")
    flags+=("--execution-timeout=")
    flags+=("--service-account-id=")
    flags+=("--package-bucket-name=")
    flags+=("--package-object-name=")
    flags+=("--package-sha256=")
    flags+=("--source-path=")
    flags+=("--source-version-id=")
    flags+=("--description=")
    flags+=("--environment=")
    flags+=("--tags=")
    flags+=("--network-name=")
    flags+=("--network-id=")
    flags+=("--subnet-name=")
    flags+=("--subnet-id=")
    flags+=("--add-service-account=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_version_set-tag()
{
    last_command="yc_serverless_function_version_set-tag"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--tag=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_version_remove-tag()
{
    last_command="yc_serverless_function_version_remove-tag"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--tag=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_version_get-by-tag()
{
    last_command="yc_serverless_function_version_get-by-tag"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--function-id=")
    flags+=("--function-name=")
    flags+=("--tag=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_version_logs()
{
    last_command="yc_serverless_function_version_logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--limit=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("-f")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_version()
{
    last_command="yc_serverless_function_version"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("create")
    commands+=("set-tag")
    commands+=("remove-tag")
    commands+=("get-by-tag")
    commands+=("logs")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_runtime_list()
{
    last_command="yc_serverless_function_runtime_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function_runtime()
{
    last_command="yc_serverless_function_runtime"

    command_aliases=()

    commands=()
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_function()
{
    last_command="yc_serverless_function"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("tag-history")
    commands+=("list-operations")
    commands+=("logs")
    commands+=("invoke")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")
    commands+=("allow-unauthenticated-invoke")
    commands+=("deny-unauthenticated-invoke")
    commands+=("version")
    commands+=("runtime")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_get()
{
    last_command="yc_serverless_trigger_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_list()
{
    last_command="yc_serverless_trigger_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_update()
{
    last_command="yc_serverless_trigger_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_add-labels()
{
    last_command="yc_serverless_trigger_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_remove-labels()
{
    last_command="yc_serverless_trigger_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_delete()
{
    last_command="yc_serverless_trigger_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_pause()
{
    last_command="yc_serverless_trigger_pause"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_resume()
{
    last_command="yc_serverless_trigger_resume"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_list-operations()
{
    last_command="yc_serverless_trigger_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_create_timer()
{
    last_command="yc_serverless_trigger_create_timer"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--cron-expression=")
    flags+=("--invoke-function-id=")
    flags+=("--invoke-function-name=")
    flags+=("--invoke-function-tag=")
    flags+=("--invoke-function-service-account-id=")
    flags+=("--invoke-function-service-account-name=")
    flags+=("--retry-attempts=")
    flags+=("--retry-interval=")
    flags+=("--dlq-queue-id=")
    flags+=("--dlq-service-account-id=")
    flags+=("--dlq-service-account-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_create_message-queue()
{
    last_command="yc_serverless_trigger_create_message-queue"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--queue=")
    flags+=("--queue-service-account-id=")
    flags+=("--queue-service-account-name=")
    flags+=("--batch-size=")
    flags+=("--batch-cutoff=")
    flags+=("--invoke-function-id=")
    flags+=("--invoke-function-name=")
    flags+=("--invoke-function-tag=")
    flags+=("--invoke-function-service-account-id=")
    flags+=("--invoke-function-service-account-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_create_internet-of-things()
{
    last_command="yc_serverless_trigger_create_internet-of-things"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--registry-id=")
    flags+=("--device-id=")
    flags+=("--mqtt-topic=")
    flags+=("--invoke-function-id=")
    flags+=("--invoke-function-name=")
    flags+=("--invoke-function-tag=")
    flags+=("--invoke-function-service-account-id=")
    flags+=("--invoke-function-service-account-name=")
    flags+=("--retry-attempts=")
    flags+=("--retry-interval=")
    flags+=("--dlq-queue-id=")
    flags+=("--dlq-service-account-id=")
    flags+=("--dlq-service-account-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_create_object-storage()
{
    last_command="yc_serverless_trigger_create_object-storage"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--bucket-id=")
    flags+=("--prefix=")
    flags+=("--suffix=")
    flags+=("--events=")
    flags+=("--invoke-function-id=")
    flags+=("--invoke-function-name=")
    flags+=("--invoke-function-tag=")
    flags+=("--invoke-function-service-account-id=")
    flags+=("--invoke-function-service-account-name=")
    flags+=("--retry-attempts=")
    flags+=("--retry-interval=")
    flags+=("--dlq-queue-id=")
    flags+=("--dlq-service-account-id=")
    flags+=("--dlq-service-account-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_create_container-registry()
{
    last_command="yc_serverless_trigger_create_container-registry"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--image-name=")
    flags+=("--tag=")
    flags+=("--events=")
    flags+=("--invoke-function-id=")
    flags+=("--invoke-function-name=")
    flags+=("--invoke-function-tag=")
    flags+=("--invoke-function-service-account-id=")
    flags+=("--invoke-function-service-account-name=")
    flags+=("--retry-attempts=")
    flags+=("--retry-interval=")
    flags+=("--dlq-queue-id=")
    flags+=("--dlq-service-account-id=")
    flags+=("--dlq-service-account-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_create_cloud-logs()
{
    last_command="yc_serverless_trigger_create_cloud-logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--log-groups=")
    flags+=("--batch-size=")
    flags+=("--batch-cutoff=")
    flags+=("--invoke-function-id=")
    flags+=("--invoke-function-name=")
    flags+=("--invoke-function-tag=")
    flags+=("--invoke-function-service-account-id=")
    flags+=("--invoke-function-service-account-name=")
    flags+=("--retry-attempts=")
    flags+=("--retry-interval=")
    flags+=("--dlq-queue-id=")
    flags+=("--dlq-service-account-id=")
    flags+=("--dlq-service-account-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger_create()
{
    last_command="yc_serverless_trigger_create"

    command_aliases=()

    commands=()
    commands+=("timer")
    commands+=("message-queue")
    commands+=("internet-of-things")
    commands+=("object-storage")
    commands+=("container-registry")
    commands+=("cloud-logs")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_trigger()
{
    last_command="yc_serverless_trigger"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("pause")
    commands+=("resume")
    commands+=("list-operations")
    commands+=("create")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_get()
{
    last_command="yc_serverless_api-gateway_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_list()
{
    last_command="yc_serverless_api-gateway_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_get-spec()
{
    last_command="yc_serverless_api-gateway_get-spec"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_create()
{
    last_command="yc_serverless_api-gateway_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--async")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--spec=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_update()
{
    last_command="yc_serverless_api-gateway_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--spec=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_delete()
{
    last_command="yc_serverless_api-gateway_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_list-operations()
{
    last_command="yc_serverless_api-gateway_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_list-access-bindings()
{
    last_command="yc_serverless_api-gateway_list-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_set-access-bindings()
{
    last_command="yc_serverless_api-gateway_set-access-bindings"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--access-binding=")
    flags+=("--yes")
    flags+=("-y")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_add-access-binding()
{
    last_command="yc_serverless_api-gateway_add-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway_remove-access-binding()
{
    last_command="yc_serverless_api-gateway_remove-access-binding"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--user-account-id=")
    flags+=("--user-yandex-login=")
    flags+=("--all-authenticated-users")
    flags+=("--subject=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless_api-gateway()
{
    last_command="yc_serverless_api-gateway"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("get-spec")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("list-operations")
    commands+=("list-access-bindings")
    commands+=("set-access-bindings")
    commands+=("add-access-binding")
    commands+=("remove-access-binding")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_serverless()
{
    last_command="yc_serverless"

    command_aliases=()

    commands=()
    commands+=("function")
    commands+=("trigger")
    commands+=("api-gateway")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_get()
{
    last_command="yc_iot_registry_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_list()
{
    last_command="yc_iot_registry_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_create()
{
    last_command="yc_iot_registry_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--certificate-file=")
    flags_with_completion+=("--certificate-file")
    flags_completion+=("__yc_handle_filename_extension_flag pem|crt")
    flags+=("--password=")
    flags+=("--read-password")
    flags+=("--generate-password")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_update()
{
    last_command="yc_iot_registry_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_logs()
{
    last_command="yc_iot_registry_logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("-f")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_add-labels()
{
    last_command="yc_iot_registry_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_remove-labels()
{
    last_command="yc_iot_registry_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_delete()
{
    last_command="yc_iot_registry_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_list-device-topic-aliases()
{
    last_command="yc_iot_registry_list-device-topic-aliases"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_certificate_list()
{
    last_command="yc_iot_registry_certificate_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_certificate_add()
{
    last_command="yc_iot_registry_certificate_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--certificate-file=")
    flags_with_completion+=("--certificate-file")
    flags_completion+=("__yc_handle_filename_extension_flag pem|crt")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_certificate_delete()
{
    last_command="yc_iot_registry_certificate_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--fingerprint=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_certificate()
{
    last_command="yc_iot_registry_certificate"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_password_list()
{
    last_command="yc_iot_registry_password_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_password_add()
{
    last_command="yc_iot_registry_password_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--password=")
    flags+=("--generate-password")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_password_delete()
{
    last_command="yc_iot_registry_password_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--password-id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry_password()
{
    last_command="yc_iot_registry_password"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_registry()
{
    last_command="yc_iot_registry"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("logs")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("list-device-topic-aliases")
    commands+=("certificate")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("certificates")
        aliashash["certificates"]="certificate"
    fi
    commands+=("password")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("passwords")
        aliashash["passwords"]="password"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_get()
{
    last_command="yc_iot_device_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_list()
{
    last_command="yc_iot_device_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_create()
{
    last_command="yc_iot_device_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--registry-id=")
    flags+=("--registry-name=")
    flags+=("--name=")
    flags+=("--description=")
    flags+=("--certificate-file=")
    flags_with_completion+=("--certificate-file")
    flags_completion+=("__yc_handle_filename_extension_flag pem|crt")
    flags+=("--topic-aliases=")
    flags+=("--password=")
    flags+=("--read-password")
    flags+=("--generate-password")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_update()
{
    last_command="yc_iot_device_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--topic-aliases=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_logs()
{
    last_command="yc_iot_device_logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--since=")
    flags+=("--until=")
    flags+=("--follow")
    flags+=("-f")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_add-topic-aliases()
{
    last_command="yc_iot_device_add-topic-aliases"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--topic-aliases=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_remove-topic-aliases()
{
    last_command="yc_iot_device_remove-topic-aliases"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--topic-aliases=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_delete()
{
    last_command="yc_iot_device_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_certificate_list()
{
    last_command="yc_iot_device_certificate_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--device-id=")
    flags+=("--device-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_certificate_add()
{
    last_command="yc_iot_device_certificate_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--device-id=")
    flags+=("--device-name=")
    flags+=("--certificate-file=")
    flags_with_completion+=("--certificate-file")
    flags_completion+=("__yc_handle_filename_extension_flag pem|crt")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_certificate_delete()
{
    last_command="yc_iot_device_certificate_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--device-id=")
    flags+=("--device-name=")
    flags+=("--fingerprint=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_certificate()
{
    last_command="yc_iot_device_certificate"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_password_list()
{
    last_command="yc_iot_device_password_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--device-id=")
    flags+=("--device-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_password_add()
{
    last_command="yc_iot_device_password_add"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--device-id=")
    flags+=("--device-name=")
    flags+=("--password=")
    flags+=("--generate-password")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_password_delete()
{
    last_command="yc_iot_device_password_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--device-id=")
    flags+=("--device-name=")
    flags+=("--password-id=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device_password()
{
    last_command="yc_iot_device_password"

    command_aliases=()

    commands=()
    commands+=("list")
    commands+=("add")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_device()
{
    last_command="yc_iot_device"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("logs")
    commands+=("add-topic-aliases")
    commands+=("remove-topic-aliases")
    commands+=("delete")
    commands+=("certificate")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("certificates")
        aliashash["certificates"]="certificate"
    fi
    commands+=("password")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("passwords")
        aliashash["passwords"]="password"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_mqtt_publish()
{
    last_command="yc_iot_mqtt_publish"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cert=")
    flags+=("--key=")
    flags+=("--username=")
    two_word_flags+=("-u")
    flags+=("--password=")
    two_word_flags+=("-P")
    flags+=("--topic=")
    two_word_flags+=("-t")
    flags+=("--qos=")
    two_word_flags+=("-q")
    flags+=("--message=")
    two_word_flags+=("-m")
    flags+=("--file=")
    two_word_flags+=("-f")
    flags+=("--stdin")
    flags+=("-l")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_mqtt_subscribe()
{
    last_command="yc_iot_mqtt_subscribe"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cert=")
    flags+=("--key=")
    flags+=("--username=")
    two_word_flags+=("-u")
    flags+=("--password=")
    two_word_flags+=("-P")
    flags+=("--topic=")
    two_word_flags+=("-t")
    flags+=("--qos=")
    two_word_flags+=("-q")
    flags+=("--details")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot_mqtt()
{
    last_command="yc_iot_mqtt"

    command_aliases=()

    commands=()
    commands+=("publish")
    commands+=("subscribe")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_iot()
{
    last_command="yc_iot"

    command_aliases=()

    commands=()
    commands+=("registry")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("registries")
        aliashash["registries"]="registry"
    fi
    commands+=("device")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("devices")
        aliashash["devices"]="device"
    fi
    commands+=("mqtt")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_get()
{
    last_command="yc_dataproc_cluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_list()
{
    last_command="yc_dataproc_cluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_create()
{
    last_command="yc_dataproc_cluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--zone=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--version=")
    flags+=("--services=")
    flags+=("--property=")
    flags+=("--ssh-public-keys-file=")
    flags+=("--subcluster=")
    flags+=("--bucket=")
    flags+=("--labels=")
    flags+=("--ui-proxy")
    flags+=("--security-group-ids=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_update()
{
    last_command="yc_dataproc_cluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--bucket=")
    flags+=("--decommission-timeout=")
    flags+=("--ui-proxy")
    flags+=("--security-group-ids=")
    flags+=("--service-account-id=")
    flags+=("--service-account-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_add-labels()
{
    last_command="yc_dataproc_cluster_add-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_remove-labels()
{
    last_command="yc_dataproc_cluster_remove-labels"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_delete()
{
    last_command="yc_dataproc_cluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--decommission-timeout=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_start()
{
    last_command="yc_dataproc_cluster_start"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_stop()
{
    last_command="yc_dataproc_cluster_stop"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--decommission-timeout=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_list-hosts()
{
    last_command="yc_dataproc_cluster_list-hosts"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_list-ui-links()
{
    last_command="yc_dataproc_cluster_list-ui-links"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster_list-operations()
{
    last_command="yc_dataproc_cluster_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_cluster()
{
    last_command="yc_dataproc_cluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("add-labels")
    commands+=("remove-labels")
    commands+=("delete")
    commands+=("start")
    commands+=("stop")
    commands+=("list-hosts")
    commands+=("list-ui-links")
    commands+=("list-operations")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_subcluster_get()
{
    last_command="yc_dataproc_subcluster_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_subcluster_list()
{
    last_command="yc_dataproc_subcluster_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_subcluster_create()
{
    last_command="yc_dataproc_subcluster_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--role=")
    flags+=("--resource-preset=")
    flags+=("--disk-type=")
    flags+=("--subnet-id=")
    flags+=("--subnet-name=")
    flags+=("--disk-size=")
    flags+=("--hosts-count=")
    flags+=("--max-hosts-count=")
    flags+=("--preemptible")
    flags+=("--warmup-duration=")
    flags+=("--stabilization-duration=")
    flags+=("--measurement-duration=")
    flags+=("--cpu-utilization-target=")
    flags+=("--autoscaling-decommission-timeout=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_subcluster_update()
{
    last_command="yc_dataproc_subcluster_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--new-name=")
    flags+=("--decommission-timeout=")
    flags+=("--resource-preset=")
    flags+=("--disk-size=")
    flags+=("--hosts-count=")
    flags+=("--max-hosts-count=")
    flags+=("--enable-preemptible")
    flags+=("--disable-preemptible")
    flags+=("--warmup-duration=")
    flags+=("--stabilization-duration=")
    flags+=("--measurement-duration=")
    flags+=("--cpu-utilization-target=")
    flags+=("--autoscaling-decommission-timeout=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_subcluster_delete()
{
    last_command="yc_dataproc_subcluster_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--decommission-timeout=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_subcluster()
{
    last_command="yc_dataproc_subcluster"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_job_get()
{
    last_command="yc_dataproc_job_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_job_list()
{
    last_command="yc_dataproc_job_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_job_log()
{
    last_command="yc_dataproc_job_log"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_job_create-spark()
{
    last_command="yc_dataproc_job_create-spark"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--main-class=")
    flags+=("--main-jar-file-uri=")
    flags+=("--jar-file-uris=")
    flags+=("--file-uris=")
    flags+=("--archive-uris=")
    flags+=("--properties=")
    flags+=("--args=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_job_create-pyspark()
{
    last_command="yc_dataproc_job_create-pyspark"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--main-python-file-uri=")
    flags+=("--python-file-uris=")
    flags+=("--jar-file-uris=")
    flags+=("--file-uris=")
    flags+=("--archive-uris=")
    flags+=("--properties=")
    flags+=("--args=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_job_create-mapreduce()
{
    last_command="yc_dataproc_job_create-mapreduce"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--main-class=")
    flags+=("--main-jar-file-uri=")
    flags+=("--jar-file-uris=")
    flags+=("--file-uris=")
    flags+=("--archive-uris=")
    flags+=("--properties=")
    flags+=("--args=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_job_create-hive()
{
    last_command="yc_dataproc_job_create-hive"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--cluster-id=")
    flags+=("--cluster-name=")
    flags+=("--name=")
    flags+=("--query-file-uri=")
    flags+=("--query-list=")
    flags+=("--continue-on-failure")
    flags+=("--script-variables=")
    flags+=("--properties=")
    flags+=("--jar-file-uris=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_job()
{
    last_command="yc_dataproc_job"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")
    commands+=("log")
    commands+=("create-spark")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("submit-spark")
        aliashash["submit-spark"]="create-spark"
    fi
    commands+=("create-pyspark")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("submit-pyspark")
        aliashash["submit-pyspark"]="create-pyspark"
    fi
    commands+=("create-mapreduce")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("submit-mapreduce")
        aliashash["submit-mapreduce"]="create-mapreduce"
    fi
    commands+=("create-hive")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("submit-hive")
        aliashash["submit-hive"]="create-hive"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_resource-preset_get()
{
    last_command="yc_dataproc_resource-preset_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_resource-preset_list()
{
    last_command="yc_dataproc_resource-preset_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc_resource-preset()
{
    last_command="yc_dataproc_resource-preset"

    command_aliases=()

    commands=()
    commands+=("get")
    commands+=("list")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_dataproc()
{
    last_command="yc_dataproc"

    command_aliases=()

    commands=()
    commands+=("cluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clusters")
        aliashash["clusters"]="cluster"
    fi
    commands+=("subcluster")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("subclusters")
        aliashash["subclusters"]="subcluster"
    fi
    commands+=("job")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("jobs")
        aliashash["jobs"]="job"
        command_aliases+=("task")
        aliashash["task"]="job"
        command_aliases+=("tasks")
        aliashash["tasks"]="job"
    fi
    commands+=("resource-preset")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource-presets")
        aliashash["resource-presets"]="resource-preset"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_get()
{
    last_command="yc_application-load-balancer_load-balancer_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_list()
{
    last_command="yc_application-load-balancer_load-balancer_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_create()
{
    last_command="yc_application-load-balancer_load-balancer_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--region-id=")
    flags+=("--network-id=")
    flags+=("--network-name=")
    flags+=("--security-group-id=")
    flags+=("--location=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_update()
{
    last_command="yc_application-load-balancer_load-balancer_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--clear-security-groups")
    flags+=("--security-group-id=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_delete()
{
    last_command="yc_application-load-balancer_load-balancer_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_add-listener()
{
    last_command="yc_application-load-balancer_load-balancer_add-listener"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--listener-name=")
    flags+=("--enable-tls")
    flags+=("--redirect-to-https")
    flags+=("--certificate-id=")
    flags+=("--allow-http10")
    flags+=("--http2-options=")
    flags+=("--external-ipv4-endpoint=")
    flags+=("--internal-ipv4-endpoint=")
    flags+=("--external-ipv6-endpoint=")
    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_update-listener()
{
    last_command="yc_application-load-balancer_load-balancer_update-listener"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--listener-name=")
    flags+=("--enable-tls")
    flags+=("--disable-tls")
    flags+=("--redirect-to-https")
    flags+=("--certificate-id=")
    flags+=("--allow-http10")
    flags+=("--http2-options=")
    flags+=("--external-ipv4-endpoint=")
    flags+=("--disable-external-ipv4-endpoint")
    flags+=("--internal-ipv4-endpoint=")
    flags+=("--disable-internal-ipv4-endpoint")
    flags+=("--external-ipv6-endpoint=")
    flags+=("--disable-external-ipv6-endpoint")
    flags+=("--clear-sni-matches")
    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_remove-listener()
{
    last_command="yc_application-load-balancer_load-balancer_remove-listener"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--listener-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_remove-sni()
{
    last_command="yc_application-load-balancer_load-balancer_remove-sni"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--listener-name=")
    flags+=("--sni-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_add-sni()
{
    last_command="yc_application-load-balancer_load-balancer_add-sni"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--listener-name=")
    flags+=("--sni-name=")
    flags+=("--server-name=")
    flags+=("--certificate-id=")
    flags+=("--allow-http10")
    flags+=("--http2-options=")
    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_update-sni()
{
    last_command="yc_application-load-balancer_load-balancer_update-sni"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--listener-name=")
    flags+=("--sni-name=")
    flags+=("--server-name=")
    flags+=("--certificate-id=")
    flags+=("--allow-http10")
    flags+=("--http2-options=")
    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_list-operations()
{
    last_command="yc_application-load-balancer_load-balancer_list-operations"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_disable-traffic()
{
    last_command="yc_application-load-balancer_load-balancer_disable-traffic"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--zone=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_enable-traffic()
{
    last_command="yc_application-load-balancer_load-balancer_enable-traffic"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--zone=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_add-location()
{
    last_command="yc_application-load-balancer_load-balancer_add-location"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--async")
    flags+=("--id=")
    flags+=("--name=")
    flags+=("--location=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_remove-location()
{
    last_command="yc_application-load-balancer_load-balancer_remove-location"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--zone=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer_target-states()
{
    last_command="yc_application-load-balancer_load-balancer_target-states"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--target-group-id=")
    flags+=("--target-group-name=")
    flags+=("--backend-group-id=")
    flags+=("--backend-group-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_load-balancer()
{
    last_command="yc_application-load-balancer_load-balancer"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("add-listener")
    commands+=("update-listener")
    commands+=("remove-listener")
    commands+=("remove-sni")
    commands+=("add-sni")
    commands+=("update-sni")
    commands+=("list-operations")
    commands+=("disable-traffic")
    commands+=("enable-traffic")
    commands+=("add-location")
    commands+=("remove-location")
    commands+=("target-states")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group_get()
{
    last_command="yc_application-load-balancer_backend-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group_list()
{
    last_command="yc_application-load-balancer_backend-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group_create()
{
    last_command="yc_application-load-balancer_backend-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group_update()
{
    last_command="yc_application-load-balancer_backend-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group_delete()
{
    last_command="yc_application-load-balancer_backend-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group_add-http-backend()
{
    last_command="yc_application-load-balancer_backend-group_add-http-backend"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backend-group-id=")
    flags+=("--backend-group-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--weight=")
    flags+=("--port=")
    flags+=("--target-group-id=")
    flags+=("--target-group-name=")
    flags+=("--panic-threshold=")
    flags+=("--locality-aware-routing-percent=")
    flags+=("--strict-locality")
    flags+=("--enable-tls")
    flags+=("--tls-sni=")
    flags+=("--tls-trusted-ca-from-file=")
    flags_with_completion+=("--tls-trusted-ca-from-file")
    flags_completion+=("_filedir")
    flags+=("--use-http2")
    flags+=("--http-healthcheck=")
    flags+=("--grpc-healthcheck=")
    flags+=("--stream-healthcheck=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group_update-http-backend()
{
    last_command="yc_application-load-balancer_backend-group_update-http-backend"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backend-group-id=")
    flags+=("--backend-group-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--weight=")
    flags+=("--port=")
    flags+=("--target-group-id=")
    flags+=("--panic-threshold=")
    flags+=("--locality-aware-routing-percent=")
    flags+=("--strict-locality")
    flags+=("--enable-tls")
    flags+=("--tls-sni=")
    flags+=("--tls-trusted-ca-from-file=")
    flags_with_completion+=("--tls-trusted-ca-from-file")
    flags_completion+=("_filedir")
    flags+=("--use-http2")
    flags+=("--http-healthcheck=")
    flags+=("--grpc-healthcheck=")
    flags+=("--stream-healthcheck=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group_delete-http-backend()
{
    last_command="yc_application-load-balancer_backend-group_delete-http-backend"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--backend-group-id=")
    flags+=("--backend-group-name=")
    flags+=("--async")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_backend-group()
{
    last_command="yc_application-load-balancer_backend-group"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("add-http-backend")
    commands+=("update-http-backend")
    commands+=("delete-http-backend")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_http-router_get()
{
    last_command="yc_application-load-balancer_http-router_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_http-router_list()
{
    last_command="yc_application-load-balancer_http-router_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_http-router_create()
{
    last_command="yc_application-load-balancer_http-router_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_http-router_update()
{
    last_command="yc_application-load-balancer_http-router_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_http-router_delete()
{
    last_command="yc_application-load-balancer_http-router_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_http-router()
{
    last_command="yc_application-load-balancer_http-router"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_get()
{
    last_command="yc_application-load-balancer_virtual-host_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_list()
{
    last_command="yc_application-load-balancer_virtual-host_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_create()
{
    last_command="yc_application-load-balancer_virtual-host_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--authority=")
    flags+=("--modify-request-header=")
    flags+=("--modify-response-header=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_update()
{
    last_command="yc_application-load-balancer_virtual-host_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--authority=")
    flags+=("--clear-authorities")
    flags+=("--modify-request-header=")
    flags+=("--clear-request-header-modifications")
    flags+=("--modify-response-header=")
    flags+=("--clear-response-header-modifications")
    flags+=("--clear-routes")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_delete()
{
    last_command="yc_application-load-balancer_virtual-host_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_append-http-route()
{
    last_command="yc_application-load-balancer_virtual-host_append-http-route"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--virtual-host-name=")
    flags+=("--match-http-method=")
    flags+=("--exact-path-match=")
    flags+=("--prefix-path-match=")
    flags+=("--backend-group-id=")
    flags+=("--backend-group-name=")
    flags+=("--request-timeout=")
    flags+=("--request-idle-timeout=")
    flags+=("--upgrade-types=")
    flags+=("--auto-host-rewrite")
    flags+=("--host-rewrite=")
    flags+=("--path-prefix-rewrite=")
    flags+=("--direct-response-code=")
    flags+=("--direct-response-body=")
    flags+=("--direct-response-body-file=")
    flags+=("--redirect-code=")
    flags+=("--redirect-scheme=")
    flags+=("--redirect-host=")
    flags+=("--redirect-port=")
    flags+=("--redirect-path=")
    flags+=("--redirect-prefix=")
    flags+=("--redirect-strip-query")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_prepend-http-route()
{
    last_command="yc_application-load-balancer_virtual-host_prepend-http-route"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--virtual-host-name=")
    flags+=("--match-http-method=")
    flags+=("--exact-path-match=")
    flags+=("--prefix-path-match=")
    flags+=("--backend-group-id=")
    flags+=("--backend-group-name=")
    flags+=("--request-timeout=")
    flags+=("--request-idle-timeout=")
    flags+=("--upgrade-types=")
    flags+=("--auto-host-rewrite")
    flags+=("--host-rewrite=")
    flags+=("--path-prefix-rewrite=")
    flags+=("--direct-response-code=")
    flags+=("--direct-response-body=")
    flags+=("--direct-response-body-file=")
    flags+=("--redirect-code=")
    flags+=("--redirect-scheme=")
    flags+=("--redirect-host=")
    flags+=("--redirect-port=")
    flags+=("--redirect-path=")
    flags+=("--redirect-prefix=")
    flags+=("--redirect-strip-query")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_remove-http-route()
{
    last_command="yc_application-load-balancer_virtual-host_remove-http-route"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--virtual-host-name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host_insert-http-route()
{
    last_command="yc_application-load-balancer_virtual-host_insert-http-route"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--http-router-id=")
    flags+=("--http-router-name=")
    flags+=("--async")
    flags+=("--virtual-host-name=")
    flags+=("--match-http-method=")
    flags+=("--exact-path-match=")
    flags+=("--prefix-path-match=")
    flags+=("--backend-group-id=")
    flags+=("--backend-group-name=")
    flags+=("--request-timeout=")
    flags+=("--request-idle-timeout=")
    flags+=("--upgrade-types=")
    flags+=("--auto-host-rewrite")
    flags+=("--host-rewrite=")
    flags+=("--path-prefix-rewrite=")
    flags+=("--direct-response-code=")
    flags+=("--direct-response-body=")
    flags+=("--direct-response-body-file=")
    flags+=("--redirect-code=")
    flags+=("--redirect-scheme=")
    flags+=("--redirect-host=")
    flags+=("--redirect-port=")
    flags+=("--redirect-path=")
    flags+=("--redirect-prefix=")
    flags+=("--redirect-strip-query")
    flags+=("--before=")
    flags+=("--after=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_virtual-host()
{
    last_command="yc_application-load-balancer_virtual-host"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("append-http-route")
    commands+=("prepend-http-route")
    commands+=("remove-http-route")
    commands+=("insert-http-route")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_target-group_get()
{
    last_command="yc_application-load-balancer_target-group_get"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_target-group_list()
{
    last_command="yc_application-load-balancer_target-group_list"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--limit=")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_target-group_create()
{
    last_command="yc_application-load-balancer_target-group_create"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--target=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_target-group_update()
{
    last_command="yc_application-load-balancer_target-group_update"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--new-name=")
    flags+=("--description=")
    flags+=("--labels=")
    flags+=("--target=")
    flags+=("--clear-targets")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_target-group_delete()
{
    last_command="yc_application-load-balancer_target-group_delete"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_target-group_add-targets()
{
    last_command="yc_application-load-balancer_target-group_add-targets"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--target=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_target-group_remove-targets()
{
    last_command="yc_application-load-balancer_target-group_remove-targets"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--id=")
    flags+=("--name=")
    flags+=("--target=")
    flags+=("--async")
    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer_target-group()
{
    last_command="yc_application-load-balancer_target-group"

    command_aliases=()

    commands=()
    commands+=("get")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("describe")
        aliashash["describe"]="get"
        command_aliases+=("show")
        aliashash["show"]="get"
    fi
    commands+=("list")
    commands+=("create")
    commands+=("update")
    commands+=("delete")
    commands+=("add-targets")
    commands+=("remove-targets")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_application-load-balancer()
{
    last_command="yc_application-load-balancer"

    command_aliases=()

    commands=()
    commands+=("load-balancer")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("lb")
        aliashash["lb"]="load-balancer"
    fi
    commands+=("backend-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("bg")
        aliashash["bg"]="backend-group"
    fi
    commands+=("http-router")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("router")
        aliashash["router"]="http-router"
    fi
    commands+=("virtual-host")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("vh")
        aliashash["vh"]="virtual-host"
    fi
    commands+=("target-group")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("tg")
        aliashash["tg"]="target-group"
    fi

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_logs()
{
    last_command="yc_logs"

    command_aliases=()

    commands=()

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

_yc_root_command()
{
    last_command="yc"

    command_aliases=()

    commands=()
    commands+=("iam")
    commands+=("resource-manager")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("resource")
        aliashash["resource"]="resource-manager"
    fi
    commands+=("compute")
    commands+=("vpc")
    commands+=("dns")
    commands+=("managed-kubernetes")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("k8s")
        aliashash["k8s"]="managed-kubernetes"
    fi
    commands+=("ydb")
    commands+=("kms")
    commands+=("certificate-manager")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("cm")
        aliashash["cm"]="certificate-manager"
    fi
    commands+=("managed-clickhouse")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("clickhouse")
        aliashash["clickhouse"]="managed-clickhouse"
    fi
    commands+=("managed-mongodb")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("mongo")
        aliashash["mongo"]="managed-mongodb"
        command_aliases+=("mongodb")
        aliashash["mongodb"]="managed-mongodb"
    fi
    commands+=("managed-mysql")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("mysql")
        aliashash["mysql"]="managed-mysql"
    fi
    commands+=("managed-sqlserver")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("sqlserver")
        aliashash["sqlserver"]="managed-sqlserver"
    fi
    commands+=("managed-postgresql")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("postgres")
        aliashash["postgres"]="managed-postgresql"
        command_aliases+=("postgresql")
        aliashash["postgresql"]="managed-postgresql"
    fi
    commands+=("managed-redis")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("redis")
        aliashash["redis"]="managed-redis"
    fi
    commands+=("managed-elasticsearch")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("elasticsearch")
        aliashash["elasticsearch"]="managed-elasticsearch"
    fi
    commands+=("managed-kafka")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("kafka")
        aliashash["kafka"]="managed-kafka"
    fi
    commands+=("container")
    commands+=("load-balancer")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("balancer")
        aliashash["balancer"]="load-balancer"
        command_aliases+=("lb")
        aliashash["lb"]="load-balancer"
    fi
    commands+=("operation")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("operations")
        aliashash["operations"]="operation"
    fi
    commands+=("config")
    commands+=("init")
    commands+=("version")
    commands+=("components")
    commands+=("serverless")
    commands+=("iot")
    commands+=("dataproc")
    commands+=("application-load-balancer")
    if [[ -z "${BASH_VERSION}" || "${BASH_VERSINFO[0]}" -gt 3 ]]; then
        command_aliases+=("alb")
        aliashash["alb"]="application-load-balancer"
    fi
    commands+=("logs")

    flags=()
    two_word_flags=()
    local_nonpersistent_flags=()
    flags_with_completion=()
    flags_completion=()

    flags+=("--profile=")
    flags+=("--debug")
    flags+=("--debug-grpc")
    flags+=("--no-user-output")
    flags+=("--retry=")
    flags+=("--cloud-id=")
    flags+=("--folder-id=")
    flags+=("--folder-name=")
    flags+=("--token=")
    flags+=("--format=")
    flags+=("--help")
    flags+=("-h")
    flags+=("--version")
    local_nonpersistent_flags+=("--version")

    must_have_one_flag=()
    must_have_one_noun=()
    noun_aliases=()
}

__start_yc()
{
    local cur prev words cword
    declare -A flaghash 2>/dev/null || :
    declare -A aliashash 2>/dev/null || :
    if declare -F _init_completion >/dev/null 2>&1; then
        _init_completion -s || return
    else
        __yc_init_completion -n "=" || return
    fi

    local c=0
    local flags=()
    local two_word_flags=()
    local local_nonpersistent_flags=()
    local flags_with_completion=()
    local flags_completion=()
    local commands=("yc")
    local must_have_one_flag=()
    local must_have_one_noun=()
    local last_command
    local nouns=()

    __yc_handle_word
}

if [[ $(type -t compopt) = "builtin" ]]; then
    complete -o default -F __start_yc yc
else
    complete -o default -o nospace -F __start_yc yc
fi

# ex: ts=4 sw=4 et filetype=sh
