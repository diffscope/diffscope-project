# Install data files
install(DIRECTORY ${CK_BUILD_MAIN_DIR}/share/
    DESTINATION ${CK_INSTALL_SHARE_DIR}
)

qm_find_qt(Core)

set(_plugins)

if(QT_VERSION_MAJOR EQUAL 5)
    # Qt5
    list(APPEND _plugins
        bearer/qgenericbearer
        iconengines/qsvgicon
        imageformats/q*
    )

    if(WIN32)
        # Windows
        list(APPEND _plugins
            platforms/qwindows
            styles/qwindowsvistastyle
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
else()
    # Qt6
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

qm_deploy_directory(${CMAKE_INSTALL_PREFIX}
    PLUGINS ${_plugins}
    LIBRARY_DIR ${_lib_dir}
    PLUGIN_DIR ${CK_INSTALL_LIBRARY_DIR}/Qt/plugins
    EXTRA_SEARCHING_PATHS ${_extra_search_path}
    VERBOSE
)

# Install vcruntime
if(MSVC AND APPLICATION_INSTALL_MSVC_RUNTIME)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${OUTPUT_DIR}")
    include(InstallRequiredSystemLibraries)
endif()