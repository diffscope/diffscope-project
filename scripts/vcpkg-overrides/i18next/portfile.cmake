set(VCPKG_BUILD_TYPE release)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_POLICY_SKIP_ARCHITECTURE_CHECK enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO i18next/i18next
    REF v26.7.1
    SHA512 5ff04250f21161375ba296f5f30346355145b6a116bfec1f9258f8c293ebaa994c45bd558adcf5e169b7a44d0c7b2ce0d942e403855ed5acbfed717172550394
    HEAD_REF master
)

set(NODE_TOOLS_DIR "${CURRENT_HOST_INSTALLED_DIR}/tools/node")
find_program(NODEJS NAMES node PATHS "${NODE_TOOLS_DIR}" "${NODE_TOOLS_DIR}/bin" NO_DEFAULT_PATH)
if(NOT NODEJS)
    message(FATAL_ERROR "node was not found. The host dependency vcpkg-tool-nodejs is required.")
endif()

if(VCPKG_HOST_IS_WINDOWS)
    set(NPM_CLI "${NODE_TOOLS_DIR}/node_modules/npm/bin/npm-cli.js")
else()
    set(NPM_CLI "${NODE_TOOLS_DIR}/lib/node_modules/npm/bin/npm-cli.js")
endif()
if(NOT EXISTS "${NPM_CLI}")
    message(FATAL_ERROR "npm-cli.js was not found at '${NPM_CLI}'.")
endif()

set(TOOLS_DIR "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
file(MAKE_DIRECTORY "${TOOLS_DIR}")
file(COPY
    "${CMAKE_CURRENT_LIST_DIR}/package.json"
    "${CMAKE_CURRENT_LIST_DIR}/rollup.config.mjs"
    DESTINATION "${TOOLS_DIR}"
)

vcpkg_execute_required_process(
    COMMAND "${NODEJS}" "${NPM_CLI}" install --no-audit --no-fund --loglevel=error
        --cache "${CURRENT_BUILDTREES_DIR}/npm-cache"
    WORKING_DIRECTORY "${TOOLS_DIR}"
    LOGNAME "npm-install-${TARGET_TRIPLET}"
)

set(MODULE_FILE "${CURRENT_PACKAGES_DIR}/share/${PORT}/${PORT}.mjs")
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/share/${PORT}")

vcpkg_execute_required_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "I18NEXT_INPUT=${SOURCE_PATH}/src/index.js"
        "I18NEXT_OUTPUT=${MODULE_FILE}"
        "${NODEJS}" "${TOOLS_DIR}/node_modules/rollup/dist/bin/rollup"
            --config "${TOOLS_DIR}/rollup.config.mjs"
    WORKING_DIRECTORY "${TOOLS_DIR}"
    LOGNAME "rollup-${TARGET_TRIPLET}"
)

if(NOT EXISTS "${MODULE_FILE}")
    message(FATAL_ERROR "The bundled i18next module was not written to '${MODULE_FILE}'.")
endif()

file(INSTALL
    "${CMAKE_CURRENT_LIST_DIR}/i18next-config.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/i18next-config-version.cmake"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
