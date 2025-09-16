project(directmanipulation
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

set(_plugin_name "Direct Manipulation")
set(_plugin_display_name DirectManipulation)

# Add target
ck_add_plugin(${PROJECT_NAME}
    NAME ${_plugin_name}
    DISPLAY_NAME ${_plugin_display_name}
    COMPAT_VERSION 0.2.0
    VENDOR "Team OpenVPI"
    DESCRIPTION "${PROJECT_DESCRIPTION}"
    MACRO_PREFIX DIRECT_MANIPULATION
)

find_package(ExtensionSystem REQUIRED)
find_package(QtWin32DirectManipulateHelper CONFIG REQUIRED)

# Configure target
file(GLOB_RECURSE _src *.h *.cpp)

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    QT_LINKS
        Core
        Gui
        Qml
        Quick
    QT_INCLUDE_PRIVATE
        Core
        Gui
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QtWin32DirectManipulateHelper::Core
        loadapi
        coreplugin
    INCLUDE_PRIVATE
        internal/**
)

# Add install command
ck_sync_include(${PROJECT_NAME}
    OPTIONS
        EXCLUDE "${PROJECT_NAME}/internal/.+"
        FORCE
)

ck_sync_include(${PROJECT_NAME}
    DIRECTORY
        internal
    PREFIX "${PROJECT_NAME}/internal"
    SKIP_INSTALL
)