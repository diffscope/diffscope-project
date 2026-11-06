# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(stdcorelib CONFIG REQUIRED)

add_library(jsonconv_stdcorelib INTERFACE)
add_library(jsonconv::stdcorelib ALIAS jsonconv_stdcorelib)

target_include_directories(jsonconv_stdcorelib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(jsonconv_stdcorelib INTERFACE Qt6::Core stdcorelib::stdcorelib)
