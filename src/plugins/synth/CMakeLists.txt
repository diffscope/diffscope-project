# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(synth
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_package(xxHash CONFIG REQUIRED)

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
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        QAKQuick
        uishell
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
        svscraft::Quick
        svscraft::QmlExtras
        SVSCraft3rdParty_tinyexpr
        xxHash::xxhash
    INCLUDE_PRIVATE
        internal/**
)
