# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

# The triplet of the build, <architecture>-<system>-<ABI>, such as x86_64-windows-msvc, which the
# about dialog shows as the target platform. The naming follows HelloUtau.
#
# The names are those that users know rather than those of vcpkg: x86_64 for the architecture
# rather than x64, a name of Microsoft, arm64 as Apple names the architecture, and mac for the
# system rather than osx.
# The ABI rather than the compiler is named, because a build of clang for the MSVC ABI loads the
# plugins of a build of MSVC.
#
# Sets the cache entry APPLICATION_TRIPLET, which -DAPPLICATION_TRIPLET=<triplet> overrides.

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64|x64)$")
    set(_application_arch x86_64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64|arm64)$")
    set(_application_arch arm64)
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _application_arch)
endif()

if(WIN32)
    set(_application_system windows)
elseif(APPLE)
    set(_application_system mac)
else()
    set(_application_system linux)
endif()

if(MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    set(_application_abi msvc)
elseif(MINGW)
    set(_application_abi mingw)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(_application_abi gcc)
else()
    string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" _application_abi)
endif()

set(APPLICATION_TRIPLET "${_application_arch}-${_application_system}-${_application_abi}" CACHE STRING
    "The triplet of the build, which the about dialog shows")

unset(_application_arch)
unset(_application_system)
unset(_application_abi)
