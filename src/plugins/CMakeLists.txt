qm_import(Translate)
qm_find_qt(LinguistTools)

set(CURRENT_PLUGIN_DESC "${APPLICATION_NAME} Application Plugin")

add_subdirectory(coreplugin)
add_subdirectory(audio)
add_subdirectory(welcomewizard)
add_subdirectory(achievement)
