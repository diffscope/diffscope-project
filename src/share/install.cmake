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

qm_deploy_directory(${CMAKE_INSTALL_PREFIX}
    PLUGINS ${_plugins}
    LIBRARY_DIR ${_lib_dir}
    PLUGIN_DIR ${CK_INSTALL_LIBRARY_DIR}/Qt/plugins
    EXTRA_SEARCHING_PATHS ${_extra_search_path}
    QML ${_qml}
    QML_DIR ${CK_INSTALL_QML_DIR}
    VERBOSE
)

# Install vcruntime
if(MSVC AND APPLICATION_INSTALL_MSVC_RUNTIME)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${OUTPUT_DIR}")
    include(InstallRequiredSystemLibraries)
endif()