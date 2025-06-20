project(loadapi VERSION 0.0.1.0)

# Add target
ck_add_library(${PROJECT_NAME} STATIC AUTOGEN)

# Configure target
file(GLOB_RECURSE _src *.h *.cpp)
qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    QT_LINKS Core Gui Widgets
    INCLUDE_PRIVATE
)

# Add install command
ck_sync_include(${PROJECT_NAME})