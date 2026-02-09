project(tst_uishell_MIDITrackSelector)

set(CMAKE_AUTOMOC ON)

file(GLOB _src *.h *.cpp)

add_executable(${PROJECT_NAME} ${_src})

qm_configure_target(${PROJECT_NAME}
    LINKS uishell
    QT_LINKS Core Gui Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)
