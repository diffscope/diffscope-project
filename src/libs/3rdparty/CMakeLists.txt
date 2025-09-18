if(APPLICATION_INSTALL AND APPLICATION_ENABLE_DEVEL)
    set(CHORUSKIT_INSTALL ON)
    set(SVSCRAFT_INSTALL ON)
    set(SCOPIC_FLOW_INSTALL ON)
    set(QACTIONKIT_INSTALL ON)
    set(TALCS_INSTALL ON)
else()
    set(CHORUSKIT_INSTALL OFF)
    set(SVSCRAFT_INSTALL OFF)
    set(SCOPIC_FLOW_INSTALL OFF)
    set(QACTIONKIT_INSTALL OFF)
    set(TALCS_INSTALL OFF)
endif()

set(CK_CMAKE_MODULES_DIR ${CMAKE_CURRENT_LIST_DIR}/choruskit/cmake PARENT_SCOPE)

add_subdirectory(choruskit)

add_subdirectory(qjsonsettings)

add_subdirectory(svscraft)

add_subdirectory(scopicflow)

add_subdirectory(qactionkit)
set(QAK_AEC_EXECUTABLE "$<TARGET_FILE:qak_aec>" PARENT_SCOPE)
set(QAK_AEC_EXECUTABLE "$<TARGET_FILE:qak_aec>")
include("${CMAKE_CURRENT_LIST_DIR}/qactionkit/src/QActionKitMacros.cmake")

add_subdirectory(talcs)