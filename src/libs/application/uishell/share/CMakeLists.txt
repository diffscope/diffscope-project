qm_import(Translate)
qm_find_qt(LinguistTools)

set(_qm_dir ${CMAKE_BUILD_SHARE_DIR}/uishell/translations)
qm_add_translation(uishell_translations
    LOCALES en_US zh_CN zh_TW ja_JP
    PREFIX uishell
    TARGETS uishell
    TS_DIR translations
    QM_DIR ${_qm_dir}
    TS_DEPENDS ChorusKit_UpdateTranslations
    QM_DEPENDS ChorusKit_ReleaseTranslations
)

if(SVSCRAFT_INSTALL)
    install(CODE "
        # execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target uishell_lupdate_task)
        execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target uishell_lrelease_task)
    ")
    install(DIRECTORY ${_qm_dir}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/uishell/translations
        FILES_MATCHING PATTERN "*.qm"
    )
endif()
