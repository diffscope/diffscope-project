# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(synth
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_package(xxHash CONFIG REQUIRED)

set(DIFFSCOPE_DSSP_CATALOG_URL "https://catalogs.diffscope.org/v1/dssp/" CACHE STRING
    "URL of the DiffSinger Synthesis Program release catalog")
set(DIFFSCOPE_DSPM_CATALOG_URL "https://catalogs.diffscope.org/v1/dspm/" CACHE STRING
    "URL of the DiffScope Package Manager release catalog")

set(_dssp_zlib_link)
if(NOT WIN32)
    find_package(ZLIB REQUIRED)
    set(_dssp_zlib_link ZLIB::ZLIB)
endif()

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.synth
    DISPLAY_NAME Synthesis
    MACRO_PREFIX SYNTH
    QML_MODULE_NAME Synth
    QT_LINKS
        Core
        Concurrent
        Gui
        Network
        Qml
        Quick
        QuickDialogs2
        Widgets
    QT_INCLUDE_PRIVATE
        Core
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        QAKQuick
        uishell
        appinstall
        transactional
        coreplugin
        audio
        dspxmodelCore
        dspxmodelORM
        dspxmodelPiece
        dini::dini
        opendspx::interpolator
        opendspx::model
        talcs::Core
        talcs::Format
        svscraft::Core
        svscraft::Gui
        svscraft::Quick
        svscraft::QmlExtras
        xxHash::xxhash
    LINKS_PRIVATE
        ${_dssp_zlib_link}
    INCLUDE_PRIVATE
        internal/**
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    DIFFSCOPE_DSSP_CATALOG_URL="${DIFFSCOPE_DSSP_CATALOG_URL}"
    DIFFSCOPE_DSPM_CATALOG_URL="${DIFFSCOPE_DSPM_CATALOG_URL}"
)
