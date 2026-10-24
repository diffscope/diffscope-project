# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

#[[
Prepares the vcpkg workspace of DiffScope in deps/: vcpkg and the overlays as the sources specify
them, and the dependencies of the manifest installed into <vcpkg>/installed.

Usage:
    cmake -P scripts/vcpkg-workspace.cmake <command> [<option>...] [-- <argument>...]

Commands:
    fetch           Checks out vcpkg and every overlay at the specified commits, and bootstraps
                    vcpkg after a checkout of another commit
    install         Runs fetch, then installs the dependencies with every overlay
    status          Lists the specified and the checked-out commit of every repository
    run <task>      Runs a task of the sources
    help            Prints this text

Options:
    --dry-run               Prints the commands that change the workspace without running them
    --triplet <name>        The triplet to install. vcpkg selects its default triplet if absent.
    --qt-dir <dir>          The Qt 6 or Qt 5 for the ports that build against Qt, as its CMake
                            directory, <Qt>/lib/cmake/Qt6 or <Qt>/lib/cmake/Qt5, or as its
                            prefix <Qt>. The environment variable QT_DIR, Qt6_DIR or Qt5_DIR
                            applies if absent, in this order.
    --sources-file <file>   A file of sources. The option may be given more than once, and a
                            file given earlier takes precedence. Without the option, the files
                            are scripts/vcpkg-sources.local.json if it exists, then
                            scripts/vcpkg-sources.json.
    --manifest-dir <dir>    The directory of vcpkg.json, scripts/vcpkg-manifest if absent

A path on the command line is relative to the current directory.

The arguments after -- are appended to the command of vcpkg install or of the task.

Sources:
    The files of sources are merged, and a file earlier in the list takes precedence over the
    files after it. vcpkg and an overlay of the same name take the members that the file of the
    highest precedence gives. The overlays of the last file keep their order, and an overlay that
    a file of higher precedence adds precedes the overlays of the files after that file. A task,
    "clone-args" and "fetch-args" at the top level are taken from the file of the highest
    precedence that gives them.

    vcpkg and the overlays of the same name in several files are one repository, which is cloned
    at most once.

    The member "path" of vcpkg or of an overlay names a local directory, relative to the file that
    gives it, which is used as it is: neither cloned, fetched nor checked out. A local vcpkg is
    bootstrapped only if it has no vcpkg executable. The file of the highest precedence that gives
    a path or a repository decides between them, and an entry that gives both uses the path.

Repositories:
    The ref of a repository is a full commit hash, a branch or a tag. A branch and a tag are
    fetched each time, and the checkout follows the commit that they name.

    A repository is created with git clone <clone-args> --no-checkout [--branch <ref>] <url> <dir>,
    with --branch for a branch or a tag, and updated with git fetch <fetch-args> origin <ref>. The
    members "clone-args" and "fetch-args" of vcpkg or of an overlay are lists of arguments for
    that repository, and the members of the same names at the top level are the lists for the
    others, each ["--quiet", "--depth", "1"] if absent. clone-args apply only to a repository that
    does not exist yet. A commit hash that is checked out is not fetched again, so other
    fetch-args apply at the next change of the ref. A shallow checkout keeps its depth unless
    fetch-args include --unshallow or --deepen.

    A repository that begins with ./ or ../ is relative to the remote origin of this repository,
    as in .gitmodules, so that it is fetched by the same protocol. A checkout whose remote
    differs, such as after a change between HTTPS and SSH, has its remote set to the resolved URL.

    On Windows, the proxy of the Internet settings applies to git and vcpkg unless the environment
    variable HTTP_PROXY or HTTPS_PROXY is set.

Overlays:
    An overlay is checked out into deps/<name>. The member "name" defaults to the last component
    of the repository without .git, or of the path, and two overlays of the same last component
    need a name of their own.

    The overlays precede vcpkg, and an overlay precedes the overlays after it. The directories
    ports and triplets of an overlay are passed to vcpkg if they exist. The members "ports" and
    "triplets" of an overlay name other directories, relative to the overlay.

Tasks:
    A task is a list of arguments in "tasks", run in the root of the repository. The variables
    ${VCPKG_ROOT}, ${VCPKG_EXECUTABLE}, ${VCPKG_INSTALLED_DIR} and ${VCPKG_MANIFEST_DIR} in an
    argument are replaced with their values. An argument that is exactly ${VCPKG_OVERLAY_ARGS} is
    replaced with the overlay arguments of vcpkg.
]]

cmake_minimum_required(VERSION 3.19)

get_filename_component(_root_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_deps_dir "${_root_dir}/deps")

# ----------------------------------
# Output and execution
# ----------------------------------
function(_vw_print _text)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_text}")
endfunction()

# Prints the command in ARGN, quoting an argument with a space, and runs it in the root of the
# repository unless the run is a dry run. Stops if the command fails.
function(_vw_execute)
    set(_text)
    foreach(_arg IN LISTS ARGN)
        if(_arg STREQUAL "" OR _arg MATCHES " ")
            set(_arg "\"${_arg}\"")
        endif()
        string(APPEND _text " ${_arg}")
    endforeach()
    _vw_print(">${_text}")

    if(_dry_run)
        return()
    endif()

    execute_process(COMMAND ${ARGN} WORKING_DIRECTORY "${_root_dir}" RESULT_VARIABLE _result)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "The command failed with ${_result}.")
    endif()
endfunction()

# Sets <out> to the output of the git command in ARGN, empty if the command fails. A query runs
# also in a dry run, because it does not change the workspace.
function(_vw_git_query _out)
    execute_process(COMMAND "${_git}" ${ARGN}
        OUTPUT_VARIABLE _output
        ERROR_QUIET
        RESULT_VARIABLE _result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT _result EQUAL 0)
        set(_output)
    endif()
    set(${_out} "${_output}" PARENT_SCOPE)
endfunction()

# ----------------------------------
# Arguments
# ----------------------------------
set(_command)
set(_task)
set(_dry_run OFF)
set(_triplet)
set(_qt_dir)
foreach(_variable IN ITEMS QT_DIR Qt6_DIR Qt5_DIR)
    if(NOT "$ENV{${_variable}}" STREQUAL "")
        set(_qt_dir "$ENV{${_variable}}")
        break()
    endif()
endforeach()
set(_source_files)
set(_manifest_dir "${_root_dir}/scripts/vcpkg-manifest")
set(_extra_args)

set(_index 0)
while(_index LESS CMAKE_ARGC)
    if(CMAKE_ARGV${_index} STREQUAL "-P")
        break()
    endif()
    math(EXPR _index "${_index} + 1")
endwhile()
math(EXPR _index "${_index} + 2")

while(_index LESS CMAKE_ARGC)
    set(_arg "${CMAKE_ARGV${_index}}")
    math(EXPR _index "${_index} + 1")

    if(_arg STREQUAL "--")
        while(_index LESS CMAKE_ARGC)
            list(APPEND _extra_args "${CMAKE_ARGV${_index}}")
            math(EXPR _index "${_index} + 1")
        endwhile()
    elseif(_arg STREQUAL "--dry-run")
        set(_dry_run ON)
    elseif(_arg MATCHES "^--(triplet|qt-dir|sources-file|manifest-dir)$")
        if(NOT _index LESS CMAKE_ARGC)
            message(FATAL_ERROR "${_arg} requires a value.")
        endif()
        set(_value "${CMAKE_ARGV${_index}}")
        math(EXPR _index "${_index} + 1")

        # A path on the command line is relative to the current directory, which is the source
        # directory of a script
        if(_arg MATCHES "^--(sources-file|manifest-dir)$")
            get_filename_component(_value "${_value}" ABSOLUTE
                BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()

        if(_arg STREQUAL "--triplet")
            set(_triplet "${_value}")
        elseif(_arg STREQUAL "--qt-dir")
            set(_qt_dir "${_value}")
        elseif(_arg STREQUAL "--sources-file")
            list(APPEND _source_files "${_value}")
        else()
            set(_manifest_dir "${_value}")
        endif()
    elseif(_arg MATCHES "^-")
        message(FATAL_ERROR "Unknown option ${_arg}. See help.")
    elseif(NOT _command)
        set(_command "${_arg}")
    elseif(_command STREQUAL "run" AND NOT _task)
        set(_task "${_arg}")
    else()
        message(FATAL_ERROR "Unexpected argument ${_arg}. See help.")
    endif()
endwhile()

if(NOT _command OR _command STREQUAL "help")
    file(READ "${CMAKE_CURRENT_LIST_FILE}" _self)
    string(REGEX MATCH "#\\[\\[\n(.*)\n\\]\\]" _ "${_self}")
    _vw_print("${CMAKE_MATCH_1}")
    return()
endif()

if(NOT _command MATCHES "^(fetch|install|status|run)$")
    message(FATAL_ERROR "Unknown command ${_command}. See help.")
endif()

if(_command STREQUAL "run" AND NOT _task)
    message(FATAL_ERROR "run requires the name of a task.")
endif()

find_program(_git NAMES git)
if(NOT _git)
    message(FATAL_ERROR "git is required and was not found.")
endif()

# ----------------------------------
# Proxy
# ----------------------------------

# Sets <out> to the proxy server of the Internet settings of Windows, empty if the proxy is
# disabled. Taken from qm_get_windows_proxy() of qmsetup. A server given for each protocol, such
# as http=host:port;https=host:port, yields the server of HTTPS, else that of HTTP.
function(_vw_get_windows_proxy _out)
    set(${_out} "" PARENT_SCOPE)
    set(_key "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")

    execute_process(COMMAND reg query "${_key}" /v ProxyEnable
        OUTPUT_VARIABLE _enable
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT _enable MATCHES "ProxyEnable[ \t\r\n]+REG_DWORD[ \t\r\n]+0x1")
        return()
    endif()

    execute_process(COMMAND reg query "${_key}" /v ProxyServer
        OUTPUT_VARIABLE _server
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT _server MATCHES "ProxyServer[ \t\r\n]+REG_SZ[ \t\r\n]+([^\r\n]*)")
        return()
    endif()
    set(_server "${CMAKE_MATCH_1}")

    if(_server MATCHES "=")
        if(_server MATCHES "(^|;)https=([^;]+)")
            set(_server "${CMAKE_MATCH_2}")
        elseif(_server MATCHES "(^|;)http=([^;]+)")
            set(_server "${CMAKE_MATCH_2}")
        else()
            return()
        endif()
    endif()

    set(${_out} "${_server}" PARENT_SCOPE)
endfunction()

if(CMAKE_HOST_WIN32 AND NOT DEFINED ENV{HTTP_PROXY} AND NOT DEFINED ENV{HTTPS_PROXY})
    _vw_get_windows_proxy(_proxy)
    if(_proxy)
        set(ENV{HTTP_PROXY} "http://${_proxy}")
        set(ENV{HTTPS_PROXY} "http://${_proxy}")
        set(ENV{ALL_PROXY} "http://${_proxy}")
        _vw_print("Proxy: http://${_proxy}")
    endif()
endif()

# ----------------------------------
# Sources
# ----------------------------------

# Sets <out> to the repository <url>. A URL that begins with ./ or ../ is relative to the URL of the
# remote origin of this repository, as a relative URL of a git submodule, so that the repository
# is fetched by the protocol of this repository. Each ../ removes the last component of the base.
function(_vw_resolve_url _url _out)
    if(NOT _url MATCHES "^\\.\\.?/")
        set(${_out} "${_url}" PARENT_SCOPE)
        return()
    endif()

    _vw_git_query(_base -C "${_root_dir}" remote get-url origin)
    if(_base STREQUAL "")
        message(FATAL_ERROR "The repository ${_url} is relative, and this repository has no "
            "remote origin to resolve it against.")
    endif()
    string(REGEX REPLACE "/+$" "" _base "${_base}")

    set(_rest "${_url}")
    while(TRUE)
        if(_rest MATCHES "^\\./(.*)$")
            set(_rest "${CMAKE_MATCH_1}")
        elseif(_rest MATCHES "^\\.\\./(.*)$")
            set(_rest "${CMAKE_MATCH_1}")
            if(NOT _base MATCHES "^(.*[/:])[^/:]+$")
                message(FATAL_ERROR "The repository ${_url} leaves the remote ${_base}.")
            endif()
            set(_base "${CMAKE_MATCH_1}")
            if(_base MATCHES "^(.*[^/])/+$")
                set(_base "${CMAKE_MATCH_1}")
            endif()
        else()
            break()
        endif()
    endwhile()

    if(_base MATCHES ":$")
        set(${_out} "${_base}${_rest}" PARENT_SCOPE)
    else()
        set(${_out} "${_base}/${_rest}" PARENT_SCOPE)
    endif()
endfunction()

# Sets <out> to the string at the path in ARGN of the JSON in the variable <json>, and <found> to
# whether the member exists.
function(_vw_json_string _json _out _found)
    string(JSON _value ERROR_VARIABLE _error GET "${${_json}}" ${ARGN})
    if(_error)
        set(${_found} OFF PARENT_SCOPE)
        set(${_out} "" PARENT_SCOPE)
    else()
        set(${_found} ON PARENT_SCOPE)
        set(${_out} "${_value}" PARENT_SCOPE)
    endif()
endfunction()

# Sets <out> to the strings of the array at the path in ARGN of the JSON in the variable <json>, and
# <found> to whether the member exists. A member that is not an array stops the run.
function(_vw_json_string_array _json _out _found)
    string(JSON _type ERROR_VARIABLE _error TYPE "${${_json}}" ${ARGN})
    if(_error)
        set(${_found} OFF PARENT_SCOPE)
        set(${_out} "" PARENT_SCOPE)
        return()
    endif()
    if(NOT _type STREQUAL "ARRAY")
        string(JOIN "." _path ${ARGN})
        message(FATAL_ERROR "${_path} of ${_source_file} is not an array of strings.")
    endif()
    string(JSON _count LENGTH "${${_json}}" ${ARGN})
    set(_items)
    if(_count GREATER 0)
        math(EXPR _last "${_count} - 1")
        foreach(_i RANGE ${_last})
            string(JSON _item GET "${${_json}}" ${ARGN} ${_i})
            list(APPEND _items "${_item}")
        endforeach()
    endif()
    set(${_found} ON PARENT_SCOPE)
    set(${_out} "${_items}" PARENT_SCOPE)
endfunction()

# Sets <out> to the last component of <location>, a URL or a path, without .git
function(_vw_default_name _location _out)
    string(REGEX REPLACE "[/\\\\]+$" "" _name "${_location}")
    string(REGEX REPLACE "^.*[/\\\\:]" "" _name "${_name}")
    string(REGEX REPLACE "\\.git$" "" _name "${_name}")
    set(${_out} "${_name}" PARENT_SCOPE)
endfunction()

# Copies the members of the entry at the path in ARGN of the current source into the variables
# _entry_<key>_<member>, and records the members of arrays in _entry_<key>_has_<member>. A path is
# made absolute against the directory of the source, and a repository is resolved.
macro(_vw_merge_entry _key)
    # A repository of a file of higher precedence replaces a path of the files merged before it,
    # unless the same entry also gives a path
    _vw_json_string(_source_json _value _found ${ARGN} repository)
    if(_found)
        _vw_resolve_url("${_value}" _entry_${_key}_url)
        set(_entry_${_key}_path)
    endif()
    _vw_json_string(_source_json _value _found ${ARGN} ref)
    if(_found)
        set(_entry_${_key}_ref "${_value}")
    endif()
    _vw_json_string(_source_json _value _found ${ARGN} path)
    if(_found)
        get_filename_component(_entry_${_key}_path "${_value}" ABSOLUTE BASE_DIR "${_source_dir}")
    endif()
    foreach(_member IN ITEMS ports triplets)
        _vw_json_string(_source_json _value _found ${ARGN} ${_member})
        if(_found)
            set(_entry_${_key}_${_member} "${_value}")
        endif()
    endforeach()
    foreach(_kind IN ITEMS clone fetch)
        _vw_json_string_array(_source_json _value _found ${ARGN} ${_kind}-args)
        if(_found)
            set(_entry_${_key}_${_kind}_args "${_value}")
            set(_entry_${_key}_has_${_kind}_args ON)
        endif()
    endforeach()
endmacro()

# The files of sources in their precedence, the local file before the sources by default
if(NOT _source_files)
    if(EXISTS "${_root_dir}/scripts/vcpkg-sources.local.json")
        list(APPEND _source_files "${_root_dir}/scripts/vcpkg-sources.local.json")
    endif()
    list(APPEND _source_files "${_root_dir}/scripts/vcpkg-sources.json")
endif()

list(LENGTH _source_files _length)
if(_length GREATER 1)
    foreach(_source_file IN LISTS _source_files)
        _vw_print("Sources: ${_source_file}")
    endforeach()
endif()

set(_default_clone_args --quiet --depth 1)
set(_default_fetch_args --quiet --depth 1)
set(_overlay_names)
set(_source_count 0)

# The files are merged from the lowest precedence, so that a file of higher precedence replaces
# the members of the files merged before it
set(_merge_order ${_source_files})
list(REVERSE _merge_order)

foreach(_source_file IN LISTS _merge_order)
    if(NOT EXISTS "${_source_file}")
        message(FATAL_ERROR "The sources ${_source_file} do not exist.")
    endif()
    math(EXPR _source_count "${_source_count} + 1")
    file(READ "${_source_file}" _source_json)
    set(_source_json_${_source_count} "${_source_json}")
    get_filename_component(_source_dir "${_source_file}" DIRECTORY)

    foreach(_kind IN ITEMS clone fetch)
        _vw_json_string_array(_source_json _value _found ${_kind}-args)
        if(_found)
            set(_default_${_kind}_args "${_value}")
        endif()
    endforeach()

    string(JSON _type ERROR_VARIABLE _error TYPE "${_source_json}" vcpkg)
    if(NOT _error)
        _vw_merge_entry(vcpkg vcpkg)
    endif()

    string(JSON _count ERROR_VARIABLE _error LENGTH "${_source_json}" overlays)
    if(_error)
        set(_count 0)
    endif()
    set(_new_names)
    set(_file_names)
    foreach(_json_index RANGE ${_count})
        if(_json_index EQUAL _count)
            break()
        endif()

        _vw_json_string(_source_json _name _found overlays ${_json_index} name)
        if(NOT _found)
            _vw_json_string(_source_json _location _found overlays ${_json_index} repository)
            if(NOT _found)
                _vw_json_string(_source_json _location _found overlays ${_json_index} path)
            endif()
            if(NOT _found)
                message(FATAL_ERROR "Overlay ${_json_index} of ${_source_file} has neither a "
                    "repository nor a path.")
            endif()
            _vw_default_name("${_location}" _name)
        endif()
        if(NOT _name MATCHES "^[A-Za-z0-9._-]+$" OR _name STREQUAL "vcpkg")
            message(FATAL_ERROR "The overlay name \"${_name}\" in ${_source_file} is not a valid "
                "directory name or is reserved for vcpkg. Give the overlay a name.")
        endif()
        if(_name IN_LIST _file_names)
            message(FATAL_ERROR "Two overlays of ${_source_file} are named \"${_name}\". Give one "
                "of them another name.")
        endif()
        list(APPEND _file_names "${_name}")
        if(NOT _name IN_LIST _overlay_names)
            list(APPEND _new_names "${_name}")
        endif()

        _vw_merge_entry(${_name} overlays ${_json_index})
    endforeach()

    # The overlays of the file of the lowest precedence keep their order, and the new overlays of
    # a file of higher precedence precede the overlays merged before them
    if(_source_count EQUAL 1)
        list(APPEND _overlay_names ${_new_names})
    else()
        list(PREPEND _overlay_names ${_new_names})
    endif()
endforeach()

# The repositories by index: 0 is vcpkg, and 1 and later are the overlays in their order.
set(_repo_name_0 vcpkg)
set(_repo_keys vcpkg ${_overlay_names})
list(LENGTH _overlay_names _repo_last)

set(_number 0)
foreach(_key IN LISTS _repo_keys)
    set(_repo_url_${_number} "${_entry_${_key}_url}")
    set(_repo_ref_${_number} "${_entry_${_key}_ref}")
    if(_number GREATER 0)
        set(_repo_name_${_number} "${_key}")
    endif()

    if(NOT "${_entry_${_key}_path}" STREQUAL "")
        set(_repo_local_${_number} ON)
        set(_repo_dir_${_number} "${_entry_${_key}_path}")
    else()
        set(_repo_local_${_number} OFF)
        set(_repo_dir_${_number} "${_deps_dir}/${_key}")
        if(_repo_url_${_number} STREQUAL "" OR _repo_ref_${_number} STREQUAL "")
            message(FATAL_ERROR "${_key} needs a repository and a ref, or a path.")
        endif()
    endif()

    foreach(_kind IN ITEMS clone fetch)
        if(_entry_${_key}_has_${_kind}_args)
            set(_repo_${_kind}_args_${_number} ${_entry_${_key}_${_kind}_args})
        else()
            set(_repo_${_kind}_args_${_number} ${_default_${_kind}_args})
        endif()
    endforeach()

    foreach(_member IN ITEMS ports triplets)
        set(_subdir "${_entry_${_key}_${_member}}")
        if(_subdir STREQUAL "")
            set(_subdir ${_member})
        endif()
        set(_repo_${_member}_${_number} "${_repo_dir_${_number}}/${_subdir}")
    endforeach()

    math(EXPR _number "${_number} + 1")
endforeach()

set(_vcpkg_dir "${_repo_dir_0}")
set(_installed_dir "${_vcpkg_dir}/installed")
if(CMAKE_HOST_WIN32)
    set(_vcpkg_executable "${_vcpkg_dir}/vcpkg.exe")
else()
    set(_vcpkg_executable "${_vcpkg_dir}/vcpkg")
endif()

# Sets <out> to whether <ref> is a full commit hash, 40 hexadecimal digits. The regular expressions
# of CMake have no repetition count.
function(_vw_is_full_hash _ref _out)
    string(LENGTH "${_ref}" _length)
    if(_ref MATCHES "^[0-9a-f]+$" AND _length EQUAL 40)
        set(${_out} ON PARENT_SCOPE)
    else()
        set(${_out} OFF PARENT_SCOPE)
    endif()
endfunction()

# A remote serves a commit by its full hash only, and an abbreviated hash cannot be fetched
foreach(_number RANGE 0 ${_repo_last})
    if(_repo_local_${_number})
        continue()
    endif()
    set(_ref "${_repo_ref_${_number}}")
    _vw_is_full_hash("${_ref}" _full)
    if(_ref MATCHES "^[0-9a-f]+$" AND NOT _full)
        message(FATAL_ERROR "The ref ${_ref} of ${_repo_name_${_number}} looks like an "
            "abbreviated commit hash. Give the full hash of 40 digits, a branch or a tag.")
    endif()
endforeach()

# Sets <out> to the arguments of vcpkg for the overlays. A directory that does not exist is left
# out, except in a dry run, which may precede the checkout.
function(_vw_overlay_args _out)
    set(_args)
    foreach(_number RANGE 1 ${_repo_last})
        if(_repo_last EQUAL 0)
            break()
        endif()
        if(_dry_run OR IS_DIRECTORY "${_repo_ports_${_number}}")
            list(APPEND _args "--overlay-ports=${_repo_ports_${_number}}")
        endif()
        if(_dry_run OR IS_DIRECTORY "${_repo_triplets_${_number}}")
            list(APPEND _args "--overlay-triplets=${_repo_triplets_${_number}}")
        endif()
    endforeach()
    set(${_out} ${_args} PARENT_SCOPE)
endfunction()

# ----------------------------------
# Commands
# ----------------------------------

# Checks out the repository of <number> at its specified commit. Sets <changed> to whether the
# checked-out commit changed. A repository with changes to tracked files is not checked out. A
# local directory is used as it is.
function(_vw_checkout _number _changed)
    set(_name "${_repo_name_${_number}}")
    set(_dir "${_repo_dir_${_number}}")
    set(_url "${_repo_url_${_number}}")
    set(_ref "${_repo_ref_${_number}}")
    set(${_changed} OFF PARENT_SCOPE)

    if(_repo_local_${_number})
        if(NOT IS_DIRECTORY "${_dir}")
            message(FATAL_ERROR "The local directory ${_dir} of ${_name} does not exist.")
        endif()
        _vw_print("${_name} uses the local directory ${_dir}.")
        return()
    endif()

    _vw_is_full_hash("${_ref}" _full)

    # The clone leaves the checkout to the steps below. git clone takes a branch or a tag but no
    # commit, so a commit is fetched after the clone of the default branch.
    if(NOT EXISTS "${_dir}/.git")
        set(_branch)
        if(NOT _full)
            set(_branch --branch "${_ref}")
        endif()
        _vw_execute("${_git}" clone ${_repo_clone_args_${_number}} --no-checkout ${_branch}
            "${_url}" "${_dir}")
        set(_head)
    else()
        _vw_git_query(_current_url -C "${_dir}" remote get-url origin)
        if(NOT _current_url STREQUAL _url)
            _vw_execute("${_git}" -C "${_dir}" remote set-url origin "${_url}")
        endif()
        _vw_git_query(_head -C "${_dir}" rev-parse HEAD)
    endif()

    # A commit hash needs no fetch if it is checked out. A branch or a tag is fetched each time,
    # and the fetched commit is compared.
    if(_full AND _head STREQUAL _ref)
        _vw_print("${_name} is at ${_ref}.")
        return()
    endif()

    _vw_execute("${_git}" -C "${_dir}" fetch ${_repo_fetch_args_${_number}} origin "${_ref}")

    if(NOT _dry_run)
        _vw_git_query(_fetched -C "${_dir}" rev-parse FETCH_HEAD)
        if(_head AND _fetched STREQUAL _head)
            _vw_print("${_name} is at ${_ref}, ${_head}.")
            return()
        endif()
    endif()

    if(_head)
        _vw_git_query(_changes -C "${_dir}" status --porcelain --untracked-files=no)
        if(_changes)
            message(FATAL_ERROR "${_dir} has changes to tracked files. Commit or discard them "
                "before ${_name} is checked out at ${_ref}.")
        endif()
    endif()

    _vw_execute("${_git}" -C "${_dir}" checkout --quiet --detach FETCH_HEAD)
    set(${_changed} ON PARENT_SCOPE)
endfunction()

function(_vw_fetch)
    file(MAKE_DIRECTORY "${_deps_dir}")

    _vw_checkout(0 _vcpkg_changed)
    foreach(_number RANGE 1 ${_repo_last})
        if(_repo_last EQUAL 0)
            break()
        endif()
        _vw_checkout(${_number} _)
    endforeach()

    # The bootstrap downloads the vcpkg tool of the checked-out commit
    if(_vcpkg_changed OR NOT EXISTS "${_vcpkg_executable}")
        if(CMAKE_HOST_WIN32)
            file(TO_NATIVE_PATH "${_vcpkg_dir}/bootstrap-vcpkg.bat" _bootstrap)
            _vw_execute(cmd /c "${_bootstrap}" -disableMetrics)
        else()
            _vw_execute(sh "${_vcpkg_dir}/bootstrap-vcpkg.sh" -disableMetrics)
        endif()
    endif()
endfunction()

# Sets <cmake_dir> to the CMake directory of the Qt in <dir>, the directory itself or
# <dir>/lib/cmake/Qt<major>, and <major> to 6 or 5 by the configuration file that it holds. A dry
# run accepts a directory without Qt as the CMake directory of Qt 6.
function(_vw_resolve_qt_dir _dir _cmake_dir _major)
    foreach(_candidate_major IN ITEMS 6 5)
        foreach(_candidate IN ITEMS "${_dir}" "${_dir}/lib/cmake/Qt${_candidate_major}")
            if(EXISTS "${_candidate}/Qt${_candidate_major}Config.cmake")
                set(${_cmake_dir} "${_candidate}" PARENT_SCOPE)
                set(${_major} ${_candidate_major} PARENT_SCOPE)
                return()
            endif()
        endforeach()
    endforeach()

    if(_dry_run)
        message(WARNING "${_dir} holds neither Qt6Config.cmake nor Qt5Config.cmake. The dry run "
            "takes it as the CMake directory of Qt 6.")
        set(${_cmake_dir} "${_dir}" PARENT_SCOPE)
        set(${_major} 6 PARENT_SCOPE)
        return()
    endif()

    message(FATAL_ERROR "${_dir} is neither the CMake directory of Qt 6 or Qt 5, which holds "
        "Qt6Config.cmake or Qt5Config.cmake, nor a Qt prefix with lib/cmake/Qt6 or lib/cmake/Qt5.")
endfunction()

function(_vw_install)
    set(_args "${_vcpkg_executable}" install
        "--x-manifest-root=${_manifest_dir}"
        "--x-install-root=${_installed_dir}"
    )
    if(_triplet)
        list(APPEND _args "--triplet=${_triplet}")
    endif()
    _vw_overlay_args(_overlay_args)
    list(APPEND _args ${_overlay_args} ${_extra_args})

    # The ports that build against Qt find it through QT_DIR and Qt<major>_DIR, which vcpkg passes
    # to them only if VCPKG_KEEP_ENV_VARS names them
    if(_qt_dir)
        set(ENV{QT_DIR} "${_qt_cmake_dir}")
        set(ENV{Qt${_qt_major}_DIR} "${_qt_cmake_dir}")
        set(_keep "$ENV{VCPKG_KEEP_ENV_VARS}")
        list(APPEND _keep QT_DIR Qt${_qt_major}_DIR)
        list(REMOVE_DUPLICATES _keep)
        string(JOIN ";" _keep_text ${_keep})
        set(ENV{VCPKG_KEEP_ENV_VARS} "${_keep_text}")
        string(CONCAT _text "Environment: QT_DIR=${_qt_cmake_dir} "
            "Qt${_qt_major}_DIR=${_qt_cmake_dir} VCPKG_KEEP_ENV_VARS=${_keep_text}")
        _vw_print("${_text}")
    else()
        message(WARNING "Neither --qt-dir nor the environment variable QT_DIR, Qt6_DIR or Qt5_DIR "
            "is set. The ports that build against Qt fail to find it.")
    endif()

    _vw_execute(${_args})
endfunction()

function(_vw_status)
    foreach(_number RANGE 0 ${_repo_last})
        set(_dir "${_repo_dir_${_number}}")
        set(_ref "${_repo_ref_${_number}}")
        if(_repo_local_${_number})
            set(_ref "local: ${_dir}")
            if(IS_DIRECTORY "${_dir}")
                _vw_git_query(_head -C "${_dir}" rev-parse HEAD)
                set(_state "local")
            else()
                set(_head "(none)")
                set(_state "missing")
            endif()
        elseif(NOT EXISTS "${_dir}/.git")
            set(_head "(none)")
            set(_state "missing")
        else()
            _vw_git_query(_head -C "${_dir}" rev-parse HEAD)
            _vw_git_query(_changes -C "${_dir}" status --porcelain --untracked-files=no)

            # A branch or a tag is resolved to its commit on the remote
            set(_commit "${_ref}")
            _vw_is_full_hash("${_ref}" _full)
            if(NOT _full)
                _vw_git_query(_remote -C "${_dir}" ls-remote origin "${_ref}")
                if(_remote MATCHES "^([0-9a-f]+)")
                    set(_commit "${CMAKE_MATCH_1}")
                    string(APPEND _ref " (${_commit})")
                endif()
            endif()

            if(_head STREQUAL _commit)
                set(_state "up to date")
            else()
                set(_state "differs")
            endif()
            if(_changes)
                string(APPEND _state ", local changes")
            endif()
        endif()
        if(_head STREQUAL "")
            set(_head "(not a git repository)")
        endif()
        string(CONCAT _text "${_repo_name_${_number}}\n    specified:   ${_ref}\n"
            "    checked out: ${_head}\n    state:       ${_state}")
        _vw_print("${_text}")
    endforeach()
endfunction()

function(_vw_run)
    # The file of the highest precedence that gives the task, which is merged last
    set(_found_json)
    set(_number ${_source_count})
    while(_number GREATER 0)
        string(JSON _type ERROR_VARIABLE _error TYPE "${_source_json_${_number}}" tasks "${_task}")
        if(NOT _error)
            if(NOT _type STREQUAL "ARRAY")
                message(FATAL_ERROR "The task \"${_task}\" is not a list of arguments.")
            endif()
            set(_found_json "${_source_json_${_number}}")
            break()
        endif()
        math(EXPR _number "${_number} - 1")
    endwhile()
    if(_found_json STREQUAL "")
        message(FATAL_ERROR "No sources give the task \"${_task}\".")
    endif()

    string(JSON _count LENGTH "${_found_json}" tasks "${_task}")
    _vw_overlay_args(_overlay_args)
    set(_args)
    math(EXPR _last "${_count} - 1")
    foreach(_i RANGE ${_last})
        string(JSON _arg GET "${_found_json}" tasks "${_task}" ${_i})
        if(_arg STREQUAL [=[${VCPKG_OVERLAY_ARGS}]=])
            list(APPEND _args ${_overlay_args})
            continue()
        endif()
        string(REPLACE [=[${VCPKG_ROOT}]=] "${_vcpkg_dir}" _arg "${_arg}")
        string(REPLACE [=[${VCPKG_EXECUTABLE}]=] "${_vcpkg_executable}" _arg "${_arg}")
        string(REPLACE [=[${VCPKG_INSTALLED_DIR}]=] "${_installed_dir}" _arg "${_arg}")
        string(REPLACE [=[${VCPKG_MANIFEST_DIR}]=] "${_manifest_dir}" _arg "${_arg}")
        list(APPEND _args "${_arg}")
    endforeach()
    list(APPEND _args ${_extra_args})

    _vw_execute(${_args})
endfunction()

if(_command STREQUAL "fetch")
    _vw_fetch()
elseif(_command STREQUAL "install")
    # Qt is resolved before the fetch, so that a wrong directory stops the run at once
    if(_qt_dir)
        _vw_resolve_qt_dir("${_qt_dir}" _qt_cmake_dir _qt_major)
    endif()
    _vw_fetch()
    _vw_install()
elseif(_command STREQUAL "status")
    _vw_status()
elseif(_command STREQUAL "run")
    _vw_run()
endif()
