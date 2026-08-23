# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(effectsunitmanager
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.effectsunitmanager
    DISPLAY_NAME "Effects Unit Manager"
    MACRO_PREFIX EFFECTS_UNIT_MANAGER
    QML_MODULE_NAME EffectsUnitManager
    QT_LINKS
        Core
        Gui
        Qml
        Quick
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        QAKCore
        QAKQuick
        uishell
        transactional
        coreplugin
        audio
        dspxmodelORM
        dspxmodelSelectionModel
        talcs::Core
        svscraft::Core
        svscraft::Quick
    INCLUDE_PRIVATE
        internal/**
)
