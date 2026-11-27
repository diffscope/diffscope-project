# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

# Use the same source revision as the Bridge PoC.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO eclipse-iceoryx/iceoryx2
    REF 135d09dd8b29f321f1725920d434864c4e512378
    SHA512 172e780842dd65ac71cbc15b08dbab05f721d1337e8c3ec04cab2214a65c54e217539b50d75ca8f31084bd1e6cd7bcbaf6020679e1d6fd692816471a0703deb6
    PATCHES
        package-static-bindings.patch
        configure-generated-header-path.patch
)

set(rust_root "${CURRENT_HOST_INSTALLED_DIR}/tools/vcpkg-tool-rust")
file(STRINGS "${rust_root}/target.txt" rust_target LIMIT_COUNT 1)
find_program(cargo NAMES cargo PATHS "${rust_root}/bin" NO_DEFAULT_PATH NO_CACHE REQUIRED)
find_program(rustc NAMES rustc PATHS "${rust_root}/bin" NO_DEFAULT_PATH NO_CACHE REQUIRED)

set(cargo_target_dir "${CURRENT_BUILDTREES_DIR}/cargo-${TARGET_TRIPLET}")
foreach(build_type IN ITEMS release debug)
    if(DEFINED VCPKG_BUILD_TYPE AND NOT VCPKG_BUILD_TYPE STREQUAL build_type)
        continue()
    endif()
    set(cargo_profile_option)
    if(build_type STREQUAL "release")
        set(cargo_profile_option --release)
    endif()
    vcpkg_execute_required_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "CARGO_HOME=${DOWNLOADS}/cargo"
            "RUSTC=${rustc}"
            "ICEORYX2_C_HEADER_DIR=${cargo_target_dir}/${rust_target}/${build_type}/iceoryx2-ffi-c-cbindgen/include"
            "${cargo}" build --locked --package iceoryx2-ffi-c
            --target "${rust_target}" --target-dir "${cargo_target_dir}" ${cargo_profile_option}
        WORKING_DIRECTORY "${SOURCE_PATH}"
        LOGNAME "cargo-${TARGET_TRIPLET}-${build_type}"
    )
endforeach()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_CXX=ON
        -DBUILD_EXAMPLES=OFF
        -DBUILD_TESTING=OFF
        -DIOX2_FEATURE_FLATBUFFERS=OFF
    OPTIONS_RELEASE "-DRUST_BUILD_ARTIFACT_PATH=${cargo_target_dir}/${rust_target}/release"
    OPTIONS_DEBUG "-DRUST_BUILD_ARTIFACT_PATH=${cargo_target_dir}/${rust_target}/debug"
)

vcpkg_cmake_install()
foreach(package IN ITEMS iceoryx2-cmake-modules iceoryx2-bb-cxx iceoryx2-c iceoryx2-cxx)
    vcpkg_cmake_config_fixup(
        PACKAGE_NAME "${package}"
        CONFIG_PATH "lib/cmake/${package}"
        DO_NOT_DELETE_PARENT_CONFIG_PATH
    )
endforeach()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
    "${CURRENT_PACKAGES_DIR}/share/doc"
)
vcpkg_install_copyright(FILE_LIST
    "${SOURCE_PATH}/LICENSE-APACHE"
    "${SOURCE_PATH}/LICENSE-MIT"
    "${SOURCE_PATH}/NOTICE.md"
)
