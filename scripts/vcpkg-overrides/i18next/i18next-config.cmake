set(i18next_MODULE_FILE "${CMAKE_CURRENT_LIST_DIR}/i18next.mjs")

if(NOT TARGET i18next::i18next)
    add_library(i18next::i18next INTERFACE IMPORTED)
    set_property(TARGET i18next::i18next PROPERTY I18NEXT_MODULE_FILE "${i18next_MODULE_FILE}")
endif()

set(i18next_VERSION "26.7.1")
