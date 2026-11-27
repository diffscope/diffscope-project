# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: LGPL-3.0-or-later

project(audiobridgeserver
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

find_package(iceoryx2-cxx 0.10.0 EXACT CONFIG REQUIRED)

if(APPLICATION_INSTALL)
    install(FILES ../../libs/3rdparty/diffscope-bridge-protocol/LICENSE
        DESTINATION "${CK_INSTALL_SHARE_DIR}/licenses"
        RENAME diffscope-bridge-protocol.txt
    )
    install(FILES "${iceoryx2-cxx_DIR}/../iceoryx2/copyright"
        DESTINATION "${CK_INSTALL_SHARE_DIR}/licenses"
        RENAME iceoryx2.txt
        OPTIONAL
    )
endif()

add_subdirectory(../../libs/3rdparty/diffscope-bridge-protocol protocol EXCLUDE_FROM_ALL)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.audiobridgeserver
    DISPLAY_NAME "Audio Bridge Server"
    MACRO_PREFIX AUDIOBRIDGESERVER
    QML_MODULE_NAME AudioBridgeServer
    QT_LINKS_PRIVATE Core Gui Network Qml Quick
    LINKS
        audio
        coreplugin
    LINKS_PRIVATE
        ChorusKit::AppCore
        DiffScopeBridgeProtocol::DiffScopeBridgeProtocol
        ExtensionSystem::ExtensionSystem
        QAKCore
        QAKQuick
        dspxproject
        iceoryx2-cxx::static-lib-cxx
        svscraft::Core
        svscraft::QmlExtras
        svscraft::Quick
        transactional
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
