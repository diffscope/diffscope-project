# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

# Icons written at build time by qiconmaker (https://github.com/stdware/qiconmaker) from images,
# so that the repository keeps the images rather than the icon files. The package qiconmaker is
# found before this file is included.

# Qt Core for the directory of its DLLs, which qiconmaker loads on Windows
qm_find_qt(Core)

# add_icon(<output> <directory>)
#
# Writes the icon <output>, an .ico of Windows or an .icns of macOS by its suffix, from the images
# <directory>/<size>x<size>.png in a custom command. The image of 1024 px is the master image. A
# size of the format that has an image of its own takes that image, and qiconmaker reduces the
# master image to every other size. A target of the same directory that lists <output> as a source
# builds the icon first, and qm_add_win_rc_enhanced() and qm_add_mac_bundle() of qmsetup accept it.
function(add_icon _output _directory)
    set(_master "${_directory}/1024x1024.png")
    set(_arguments "${_output}" "${_master}")
    set(_depends "${_master}")

    # The sizes that qiconmaker writes for each format by default
    if(_output MATCHES "\\.icns$")
        set(_sizes 16 32 64 128 256 512)
    else()
        set(_sizes 16 20 24 30 32 36 40 48 60 64 72 80 96 256)
    endif()

    foreach(_size IN LISTS _sizes)
        set(_file "${_directory}/${_size}x${_size}.png")
        if(EXISTS "${_file}")
            list(APPEND _arguments "${_size}=${_file}")
            list(APPEND _depends "${_file}")
        endif()
    endforeach()

    # The program loads the DLLs of Qt, which Windows finds through PATH. On the other systems the
    # program holds the run path to the libraries of Qt.
    set(_command $<TARGET_FILE:qiconmaker::qiconmaker>)
    if(WIN32)
        set(_command ${CMAKE_COMMAND} -E env "PATH=$<TARGET_FILE_DIR:Qt6::Core>" ${_command})
    endif()
    add_custom_command(OUTPUT "${_output}"
        COMMAND ${_command} ${_arguments}
        DEPENDS ${_depends}
        COMMENT "Writing ${_output}"
        VERBATIM
    )
endfunction()
