project(audio
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

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
        opendspx::interpolator
        opendspx::model
        talcs::Core
        talcs::Device
        talcs::Dspx
        talcs::Format
        talcs::Widgets
    INCLUDE_PRIVATE
        internal/**
)

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
