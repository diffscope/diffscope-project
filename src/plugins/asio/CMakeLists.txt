# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: GPL-3.0-only

project(asio
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.asio
    DISPLAY_NAME ASIO
    MACRO_PREFIX ASIO
    NO_TRANSLATION
    NO_QML_MODULE
    NO_ACTION
    QT_LINKS
        Core
    LINKS
        ExtensionSystem::ExtensionSystem
        audio
        talcs::Asio
    INCLUDE_PRIVATE
        internal/**
)
