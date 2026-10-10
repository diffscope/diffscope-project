# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

# Install data files
install(DIRECTORY ${CK_BUILD_MAIN_DIR}/share/
    DESTINATION ${CK_INSTALL_SHARE_DIR}
)

qm_find_qt(Core)

set(_plugins
    generic/qtuiotouchplugin
    iconengines/qsvgicon
    imageformats/q*
    networkinformation/qnetworklistmanager
    tls/q*
)

if(WIN32)
    # Windows
    list(APPEND _plugins
        platforms/qwindows
        styles/qmodernwindowsstyle
    )
elseif(APPLE)
    # Mac
    list(APPEND _plugins
        platforms/qcocoa
        styles/qmacstyle
        platforminputcontexts/q*
        printsupport/q*
        virtualkeyboard/q*
    )
else()
    # Linux
    list(APPEND _plugins
        platforms/qxcb
        platforminputcontexts/*
        xcbglintegrations/*
    )
endif()

if(WIN32)
    set(_lib_dir ${CK_INSTALL_RUNTIME_DIR})
else()
    set(_lib_dir ${CK_INSTALL_LIBRARY_DIR})
endif()

if(WIN32)
    set(_extra_search_path ${CK_BUILD_RUNTIME_DIR})
else()
    set(_extra_search_path ${CK_BUILD_LIBRARY_DIR})
endif()

set(_qml
    Qt/labs
    Qt5Compat
    QtCore
    QtQml
    QtQuick
)

set(_vcpkg_lib_dir)

if(UNIX AND NOT APPLE AND DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(_vcpkg_lib_dir ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib)
endif()

if(_vcpkg_lib_dir)
    # The deployment resolves each binary with `ldd`, so Qt picks up the system glib while gio and
    # gobject from vcpkg pick up the newer vcpkg glib, and the two end up mixed in one directory.
    # Let the loader prefer vcpkg libraries so that every library comes from a single place.
    install(CODE "
        set(_old_ld_library_path \"\$ENV{LD_LIBRARY_PATH}\")
        set(ENV{LD_LIBRARY_PATH} \"${_vcpkg_lib_dir}:\$ENV{LD_LIBRARY_PATH}\")
    ")
endif()

qm_deploy_directory(${CMAKE_INSTALL_PREFIX}
    PLUGINS ${_plugins}
    LIBRARY_DIR ${_lib_dir}
    PLUGIN_DIR ${CK_INSTALL_LIBRARY_DIR}/Qt/plugins
    EXTRA_SEARCHING_PATHS ${_extra_search_path}
    QML ${_qml}
    QML_DIR ${CK_INSTALL_QML_DIR}
    VERBOSE
)

if(_vcpkg_lib_dir)
    install(CODE "set(ENV{LD_LIBRARY_PATH} \"\${_old_ld_library_path}\")")
endif()

# Install vcruntime
if(MSVC AND APPLICATION_INSTALL_MSVC_RUNTIME)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${OUTPUT_DIR}")
    include(InstallRequiredSystemLibraries)
endif()

if(APPLE)
    install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/qml/
        DESTINATION ${CK_INSTALL_QML_DIR}
        USE_SOURCE_PERMISSIONS
    )
endif()
