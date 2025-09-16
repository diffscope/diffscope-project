qm_import(Translate)
qm_find_qt(LinguistTools)

set(CURRENT_PLUGIN_DESC "${PROJECT_NAME} Application Plugin")

add_subdirectory(coreplugin)
add_subdirectory(audio)
add_subdirectory(achievement)
add_subdirectory(welcomewizard)
if(WIN32)
    add_subdirectory(directmanipulation)
endif()