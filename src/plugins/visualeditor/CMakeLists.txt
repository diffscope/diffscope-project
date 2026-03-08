project(visualeditor
    VERSION ${APPLICATION_VERSION}
    DESCRIPTION ${CURRENT_PLUGIN_DESC}
)

diffscope_add_builtin_plugin(${PROJECT_NAME}
    NAME org.diffscope.visualeditor
    DISPLAY_NAME "Visual Editor"
    MACRO_PREFIX VISUAL_EDITOR
    QML_MODULE_NAME VisualEditor
    QT_LINKS
        Core
        Gui
        Widgets
        Qml
        Quick
        QuickControls2
        QuickTemplates2
        QuickDialogs2
        StateMachine
    QT_INCLUDE_PRIVATE
        Core
        Gui
        Widgets
        Qml
        Quick
        QuickControls2
        QuickTemplates2
    LINKS
        ExtensionSystem::ExtensionSystem
        ChorusKit::AppCore
        uishell
        QAKCore
        QAKQuick
        svscraft::QmlExtras
        dspxmodel
        coreplugin
        opendspx::model
        opendspx::serializer
        ScopicFlowCore
        ScopicFlowViews
    INCLUDE_PRIVATE
        internal/**
)

file(GLOB _res_icons res/icons/*)

qt_add_resources(${PROJECT_NAME} icons
    PREFIX /diffscope/visualeditor/icons
    BASE res/icons
    FILES ${_res_icons}
)
