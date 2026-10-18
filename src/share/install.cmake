# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

# Install data files
install(DIRECTORY ${CK_BUILD_MAIN_DIR}/share/
    DESTINATION ${CK_INSTALL_SHARE_DIR}
)

qm_find_qt(Core)

# QT_PLUGINS deploys the plugins of every Qt module in the deployment. The OpenSSL backend requires
# OpenSSL, which the deployment does not include. The QML debugging plugins serve development only.
# Qt SQL is in the deployment only as a dependency of Qt Quick Local Storage, which DiffScope does
# not use, therefore the SQL drivers are excluded with the module.
set(_qt_plugins_exclude
    generic/qinsighttracker
    tls/qopensslbackend
    qmltooling/qmldbg_debugger
    qmltooling/qmldbg_inspector
    qmltooling/qmldbg_local
    qmltooling/qmldbg_messages
    qmltooling/qmldbg_native
    qmltooling/qmldbg_nativedebugger
    qmltooling/qmldbg_preview
    qmltooling/qmldbg_profiler
    qmltooling/qmldbg_quickeventreplay
    qmltooling/qmldbg_quickprofiler
    qmltooling/qmldbg_server
    qmltooling/qmldbg_tcp
)

set(_qt_plugins_exclude_modules
    Sql
)

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

set(_exclude)

if(UNIX AND NOT APPLE)
    # Libraries resolved from the system library directories (glibc, graphics drivers, X11, D-Bus,
    # ...) must come from the target system. Qt and vcpkg libraries resolve from their own
    # directories and are still deployed.
    list(APPEND _exclude "^/(usr/)?lib(64)?/")
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
    QT_PLUGINS
    QT_PLUGINS_EXCLUDE ${_qt_plugins_exclude}
    QT_PLUGINS_EXCLUDE_MODULES ${_qt_plugins_exclude_modules}
    LIBRARY_DIR ${_lib_dir}
    PLUGIN_DIR ${CK_INSTALL_LIBRARY_DIR}/Qt/plugins
    EXTRA_SEARCHING_PATHS ${_extra_search_path}
    EXCLUDE ${_exclude}
    QML ${_qml}
    QML_DIR ${CK_INSTALL_QML_DIR}
    VERBOSE
)

if(_vcpkg_lib_dir)
    install(CODE "set(ENV{LD_LIBRARY_PATH} \"\${_old_ld_library_path}\")")
endif()

if(APPLE)
    install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/qml/
        DESTINATION ${CK_INSTALL_QML_DIR}
        USE_SOURCE_PERMISSIONS
    )
endif()
