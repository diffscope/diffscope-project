# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: GPL-3.0-only

project(libresvipformatconverter
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

set(DIFFSCOPE_LIBRESVIP_HOMEPAGE_URL "https://soulmelody.github.io/LibreSVIP/" CACHE STRING
    "Homepage URL of LibreSVIP")
set(DIFFSCOPE_LIBRESVIP_CATALOG_BASE_URL "https://catalogs.diffscope.org/v1/libresvip-cli/" CACHE STRING
    "Base directory URL of the LibreSVIP release catalog")

qm_find_qt(Network Protobuf Grpc)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.libresvipformatconverter
    DISPLAY_NAME "LibreSVIP Format Converter"
    MACRO_PREFIX LIBRESVIP_FORMAT_CONVERTER
    QML_MODULE_NAME LibreSVIPFormatConverter
    NO_ACTION
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
        Network
        Protobuf
        Grpc
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        uishell
        appinstall
        coreplugin
        importexportmanager
        opendspx::model
        opendspx::serializer
        svscraft::Core
        svscraft::Quick
    INCLUDE_PRIVATE
        internal/**
)

qt_add_protobuf(${PROJECT_NAME}
    PROTO_FILES
        proto/libresvip.proto
)

qt_add_grpc(${PROJECT_NAME} CLIENT
    PROTO_FILES
        proto/libresvip.proto
)

qt_add_resources(${PROJECT_NAME} libresvipformatconverter_resources
    PREFIX "/libresvipformatconverter"
    FILES
        res/libresvip/libresvip.ico
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    DIFFSCOPE_LIBRESVIP_CATALOG_BASE_URL="${DIFFSCOPE_LIBRESVIP_CATALOG_BASE_URL}"
    DIFFSCOPE_LIBRESVIP_HOMEPAGE_URL="${DIFFSCOPE_LIBRESVIP_HOMEPAGE_URL}"
)
