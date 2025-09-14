if(APPLICATION_INSTALL AND APPLICATION_ENABLE_DEVEL)
    set(CHORUSKIT_INSTALL on)
else()
    set(CHORUSKIT_INSTALL off)
endif()

set(CK_CMAKE_MODULES_DIR ${CMAKE_CURRENT_LIST_DIR}/choruskit/cmake PARENT_SCOPE)

add_subdirectory(choruskit)

add_subdirectory(qjsonsettings)

set(SVSCRAFT_INSTALL off)
add_subdirectory(svscraft)

set(SCOPIC_FLOW_INSTALL off)
add_subdirectory(scopicflow)

add_subdirectory(qactionkit)
set(QAK_AEC_EXECUTABLE "$<TARGET_FILE:qak_aec>" PARENT_SCOPE)
set(QAK_AEC_EXECUTABLE "$<TARGET_FILE:qak_aec>")
include("${CMAKE_CURRENT_LIST_DIR}/qactionkit/src/QActionKitMacros.cmake")

add_subdirectory(talcs)