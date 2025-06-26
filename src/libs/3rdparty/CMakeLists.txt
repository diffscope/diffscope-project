if(APPLICATION_INSTALL AND APPLICATION_ENABLE_DEVEL)
    set(CHORUSKIT_INSTALL on)
else()
    set(CHORUSKIT_INSTALL off)
endif()

set(CK_CMAKE_MODULES_DIR ${CMAKE_CURRENT_LIST_DIR}/choruskit/cmake PARENT_SCOPE)

add_subdirectory(choruskit)

add_subdirectory(qjsonsettings)