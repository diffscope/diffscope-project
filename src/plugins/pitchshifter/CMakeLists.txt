# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: GPL-3.0-or-later

project(pitchshifter
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_package(PkgConfig REQUIRED)
find_package(xxHash CONFIG REQUIRED)
pkg_check_modules(RUBBERBAND REQUIRED IMPORTED_TARGET rubberband)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.pitchshifter
    DISPLAY_NAME "Pitch Shifter"
    MACRO_PREFIX PITCH_SHIFTER
    QML_MODULE_NAME PitchShifter
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        QAKQuick
        coreplugin
        audio
        dspxmodelORM
        talcs::Format
        transactional
        svscraft::Core
        svscraft::Quick
        PkgConfig::RUBBERBAND
        xxHash::xxhash
    INCLUDE_PRIVATE
        internal/**
)
