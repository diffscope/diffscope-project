# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

qm_import(QtLinguist)
qm_find_qt(LinguistTools)

set(_qm_dir ${CMAKE_BUILD_SHARE_DIR}/singercatalog/translations)
qm_add_translation(singercatalog_translations
    LOCALES en_US zh_CN zh_TW ja_JP
    PREFIX singercatalog
    TARGETS singercatalog
    TS_DIR translations
    QM_DIR ${_qm_dir}
    TS_DEPENDS ChorusKit_UpdateTranslations
    QM_DEPENDS ChorusKit_ReleaseTranslations
)

if(APPLICATION_INSTALL)
    install(DIRECTORY ${_qm_dir}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/singercatalog/translations
        FILES_MATCHING PATTERN "*.qm"
    )
endif()
