# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

qm_find_qt(Core)
get_filename_component(_qt_translations_dir "${QT6_INSTALL_TRANSLATIONS}" ABSOLUTE
    BASE_DIR "${QT6_INSTALL_PREFIX}"
)

qm_add_copy_command(DiffScope_CopyQtTranslations
    SOURCES
        "${_qt_translations_dir}/qtbase_*.qm"
        "${_qt_translations_dir}/qtdeclarative_*.qm"
    DESTINATION "${CMAKE_BUILD_SHARE_DIR}/Qt/translations"
)
add_dependencies(${CK_APPLICATION_TARGET} DiffScope_CopyQtTranslations)
