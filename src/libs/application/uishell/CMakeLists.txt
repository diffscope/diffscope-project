# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

add_subdirectory(src)
add_subdirectory(share)

if(APPLICATION_BUILD_TESTS)
    add_subdirectory(tests)
endif()