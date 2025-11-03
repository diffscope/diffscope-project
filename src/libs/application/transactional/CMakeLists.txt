project(transactional VERSION ${APPLICATION_VERSION})

file(GLOB_RECURSE _src *.h *.cpp)

ck_add_library(${PROJECT_NAME} STATIC AUTOGEN)

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    LINKS
    QT_LINKS Core
    QT_INCLUDE_PRIVATE Core
    INCLUDE_PRIVATE *
    PREFIX TRANSACTIONAL
)

ck_sync_include(${PROJECT_NAME} FORCE)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)
