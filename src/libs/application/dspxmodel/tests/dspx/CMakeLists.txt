project(tst_dspxmodel_dspx)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

file(GLOB _src *.h *.cpp)

add_executable(${PROJECT_NAME} ${_src})

qm_configure_target(${PROJECT_NAME}
    LINKS opendspx dspxmodel
    QT_LINKS Core
)

qt_add_resources(${PROJECT_NAME} dspx_file
    PREFIX /tst_dspxmodel_dspx
    BASE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES test.dspx
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)