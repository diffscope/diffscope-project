# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(audio
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_package(xxHash CONFIG REQUIRED)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.audio
    DISPLAY_NAME Audio
    MACRO_PREFIX AUDIO
    QML_MODULE_NAME Audio
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        uishell
        QAKCore
        QAKQuick
        coreplugin
        dspxmodelORM
        dspxmodelSelectionModel
        opendspx::interpolator
        opendspx::model
        talcs::Core
        talcs::Midi
        talcs::Device
        talcs::Dspx
        talcs::Format
        talcs::Widgets
        transactional
        svscraft::Core
        svscraft::Quick
        xxHash::xxhash
    INCLUDE_PRIVATE
        internal/**
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_dependencies(${PROJECT_NAME} TalcsPipeWire TalcsPulseAudio TalcsJack TalcsAlsa)
    file(RELATIVE_PATH _talcs_library_dir "/${CK_INSTALL_PLUGINS_DIR}/audio" "/${CK_INSTALL_LIBRARY_DIR}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE AUDIO_TALCS_LIBRARY_RELATIVE_DIR="${_talcs_library_dir}")
endif()

file(GLOB _res_soundfx res/soundfx/*)
qt_add_resources(${PROJECT_NAME} soundfx
    PREFIX /diffscope/audio/soundfx
    BASE res/soundfx
    FILES ${_res_soundfx}
)

file(GLOB _res_singeravatar res/singeravatar/*)
qt_add_resources(${PROJECT_NAME} singeravatar
    PREFIX /diffscope/audio/singeravatar
    BASE res/singeravatar
    FILES ${_res_singeravatar}
)
