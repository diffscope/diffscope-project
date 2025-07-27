import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

Window {
    width: 800
    height: 600
    visible: true
    QtObject {
        id: plugin1
        readonly property string name: "plugin1"
        readonly property string displayName: "Plugin 1"
        readonly property string version: "0.0.1.0"
        readonly property string compatVersion: "0.0.0.0"
        readonly property string vendor: "Vendor 1"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Description text"
        readonly property string url: "https://example.com/"
        readonly property string category: "Test 1"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: true
        readonly property bool enabledIndirectly: false
        readonly property bool forceEnabled: false
        readonly property bool forceDisabled: false
        readonly property bool hasError: false
        readonly property string errorString: ""
        property bool enabledBySettings: true
        readonly property bool initialEnabledBySettings: true
        readonly property string filePath: "/path/to/plugin1/plugin1.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: true
        readonly property list<QtObject> dependencies: []
        readonly property list<QtObject> dependents: [plugin2, plugin6, plugin3, plugin4, plugin7, plugin5, plugin8, plugin9]
    }
    QtObject {
        id: plugin2
        readonly property string name: "plugin2"
        readonly property string displayName: "Plugin 2"
        readonly property string version: "0.1.2.0"
        readonly property string compatVersion: "0.0.0.0"
        readonly property string vendor: "Vendor 2"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
        readonly property string url: "https://example.com/"
        readonly property string category: "Test 1"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: false
        readonly property bool enabledIndirectly: true
        readonly property bool forceEnabled: true
        readonly property bool forceDisabled: false
        readonly property bool hasError: true
        readonly property string errorString: "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
        property bool enabledBySettings: true
        readonly property bool initialEnabledBySettings: true
        readonly property string filePath: "/path/to/plugin2/plugin2.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: false
        readonly property list<QtObject> dependencies: [plugin1]
        readonly property list<QtObject> dependents: [plugin3, plugin5, plugin4, plugin7, plugin8]
    }
    QtObject {
        id: plugin3
        readonly property string name: "plugin3"
        readonly property string displayName: "Plugin 3"
        readonly property string version: "1.0.0.0"
        readonly property string compatVersion: "1.0.0.0"
        readonly property string vendor: "Vendor 3"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Plugin 3 depends on Plugin 1 and Plugin 2."
        readonly property string url: ""
        readonly property string category: "Test 2"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: false
        readonly property bool enabledIndirectly: false
        readonly property bool forceEnabled: false
        readonly property bool forceDisabled: false
        readonly property bool hasError: false
        readonly property string errorString: ""
        property bool enabledBySettings: true
        readonly property bool initialEnabledBySettings: true
        readonly property string filePath: "/path/to/plugin3/plugin3.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: true
        readonly property list<QtObject> dependencies: [plugin1, plugin2]
        readonly property list<QtObject> dependents: [plugin4, plugin7, plugin8]
    }

    QtObject {
        id: plugin4
        readonly property string name: "plugin4"
        readonly property string displayName: "Plugin 4"
        readonly property string version: "2.0.0.0"
        readonly property string compatVersion: "2.0.0.0"
        readonly property string vendor: "Vendor 4"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Plugin 4 depends on Plugin 3."
        readonly property string url: "https://example.com/"
        readonly property string category: "Test 2"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: false
        readonly property bool enabledIndirectly: false
        readonly property bool forceEnabled: false
        readonly property bool forceDisabled: false
        readonly property bool hasError: false
        readonly property string errorString: ""
        property bool enabledBySettings: true
        readonly property bool initialEnabledBySettings: true
        readonly property string filePath: "/path/to/plugin4/plugin4.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: true
        readonly property list<QtObject> dependencies: [plugin3, plugin1, plugin2]
        readonly property list<QtObject> dependents: [plugin8]
    }

    QtObject {
        id: plugin5
        readonly property string name: "plugin5"
        readonly property string displayName: "Plugin 5"
        readonly property string version: "3.0.0.0"
        readonly property string compatVersion: "3.0.0.0"
        readonly property string vendor: "Vendor 5"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Plugin 5 depends on Plugin 2."
        readonly property string url: ""
        readonly property string category: "Test 3"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: false
        readonly property bool enabledIndirectly: false
        readonly property bool forceEnabled: false
        readonly property bool forceDisabled: false
        readonly property bool hasError: false
        readonly property string errorString: ""
        property bool enabledBySettings: true
        readonly property bool initialEnabledBySettings: true
        readonly property string filePath: "/path/to/plugin5/plugin5.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: true
        readonly property list<QtObject> dependencies: [plugin2, plugin1]
        readonly property list<QtObject> dependents: [plugin7, plugin8, plugin9]
    }

    QtObject {
        id: plugin6
        readonly property string name: "plugin6"
        readonly property string displayName: "Plugin 6"
        readonly property string version: "4.0.0.0"
        readonly property string compatVersion: "4.0.0.0"
        readonly property string vendor: "Vendor 6"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Plugin 6 depends on Plugin 1 and is depended on by Plugin 2."
        readonly property string url: "https://example.com/"
        readonly property string category: "Test 1"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: false
        readonly property bool enabledIndirectly: false
        readonly property bool forceEnabled: false
        readonly property bool forceDisabled: false
        readonly property bool hasError: false
        readonly property string errorString: ""
        property bool enabledBySettings: false
        readonly property bool initialEnabledBySettings: false
        readonly property string filePath: "/path/to/plugin6/plugin6.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: false
        readonly property list<QtObject> dependencies: [plugin1]
        readonly property list<QtObject> dependents: [plugin9]
    }

    QtObject {
        id: plugin7
        readonly property string name: "plugin7"
        readonly property string displayName: "Plugin 7"
        readonly property string version: "5.0.0.0"
        readonly property string compatVersion: "5.0.0.0"
        readonly property string vendor: "Vendor 7"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Plugin 7 depends on Plugin 3 and Plugin 5, and is depended on by Plugin 8 and Plugin 9."
        readonly property string url: "https://example.com/"
        readonly property string category: "Test 3"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: false
        readonly property bool enabledIndirectly: false
        readonly property bool forceEnabled: false
        readonly property bool forceDisabled: false
        readonly property bool hasError: false
        readonly property string errorString: ""
        property bool enabledBySettings: false
        readonly property bool initialEnabledBySettings: false
        readonly property string filePath: "/path/to/plugin7/plugin7.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: false
        readonly property list<QtObject> dependencies: [plugin3, plugin5, plugin1, plugin2]
        readonly property list<QtObject> dependents: [plugin8, plugin9]
    }

    QtObject {
        id: plugin8
        readonly property string name: "plugin8"
        readonly property string displayName: "Plugin 8"
        readonly property string version: "6.0.0.0"
        readonly property string compatVersion: "6.0.0.0"
        readonly property string vendor: "Vendor 8"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Plugin 8 depends on Plugin 7 and Plugin 4."
        readonly property string url: "https://example.com/"
        readonly property string category: "Test 2"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: false
        readonly property bool enabledIndirectly: false
        readonly property bool forceEnabled: false
        readonly property bool forceDisabled: false
        readonly property bool hasError: false
        readonly property string errorString: ""
        property bool enabledBySettings: false
        readonly property bool initialEnabledBySettings: false
        readonly property string filePath: "/path/to/plugin8/plugin8.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: false
        readonly property list<QtObject> dependencies: [plugin7, plugin4, plugin3, plugin5, plugin1, plugin2]
        readonly property list<QtObject> dependents: []
    }

    QtObject {
        id: plugin9
        readonly property string name: "plugin9"
        readonly property string displayName: "Plugin 9"
        readonly property string version: "7.0.0.0"
        readonly property string compatVersion: "7.0.0.0"
        readonly property string vendor: "Vendor 9"
        readonly property string copyright: "Copyright text"
        readonly property string license: "License text"
        readonly property string description: "Plugin 9 depends on Plugin 7 and Plugin 6."
        readonly property string url: ""
        readonly property string category: "Test 1"
        readonly property bool availableForHostPlatform: true
        readonly property bool required: false
        readonly property bool enabledIndirectly: false
        readonly property bool forceEnabled: false
        readonly property bool forceDisabled: false
        readonly property bool hasError: false
        readonly property string errorString: ""
        property bool enabledBySettings: false
        readonly property bool initialEnabledBySettings: false
        readonly property string filePath: "/path/to/plugin9/plugin9.so"
        readonly property bool restartRequired: enabledBySettings !== initialEnabledBySettings
        readonly property bool running: false
        readonly property list<QtObject> dependencies: [plugin7, plugin6, plugin3, plugin5, plugin1, plugin2]
        readonly property list<QtObject> dependents: []
    }
    PluginView {
        anchors.fill: parent
        provider: QtObject {
            id: pluginManagerHelper
            property var pluginCollections: [
                {
                    name: "Test 1",
                    plugins: [
                        plugin1,
                        plugin2,
                        plugin6,
                        plugin9,
                    ],
                },
                {
                    name: "Test 2",
                    plugins: [
                        plugin3,
                        plugin4,
                        plugin8,
                    ],
                },
                {
                    name: "Test 3",
                    plugins: [
                        plugin5,
                        plugin7,
                    ],
                },
            ]
        }
    }
}