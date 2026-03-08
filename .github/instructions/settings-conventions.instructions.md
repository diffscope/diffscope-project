# Settings Conventions

This document outlines the conventions for using QSettings in the DiffScope project for configuration storage and persistent key-value data.

## QSettings Access

### C++ Settings Access

Use `RuntimeInterface` to access QSettings instances:

```cpp
#include <coreplugin/runtimeinterface.h>

// User-scoped settings (stored per user)
QSettings *userSettings = Core::RuntimeInterface::settings();

// Global settings (system-wide)
QSettings *globalSettings = Core::RuntimeInterface::globalSettings();
```

### Settings Groups

When reading or writing settings, always use groups to organize settings by class or module:

```cpp
// For C++ classes: use the class name as group
class ProjectWindowWorkspaceManager : public QObject {
private:
    void saveSettings() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup("ProjectWindowWorkspaceManager");
        settings->setValue("currentLayout", m_currentLayout.toVariant());
        settings->endGroup();
    }
    
    void loadSettings() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup("ProjectWindowWorkspaceManager");
        auto layout = settings->value("currentLayout").toMap();
        settings->endGroup();
    }
};
```

### Group Naming Conventions

- **C++ Classes**: Use the class name directly
  ```cpp
  settings->beginGroup("NotificationCenter");
  settings->beginGroup("ProjectWindowWorkspaceManager");
  ```

- **QML Components**: Use module URI + type name
  ```cpp
  // For DiffScope.CorePlugin.SomeComponent
  settings->beginGroup("DiffScope.CorePlugin.SomeComponent");
  
  // For SVSCraft.UIComponents.ThemedWindow
  settings->beginGroup("SVSCraft.UIComponents.ThemedWindow");
  ```

## QML Settings Integration

### Using SVSCraft.Extras.Settings

In QML, use the `Settings` component from `SVSCraft.Extras`:

```qml
import SVSCraft.Extras

Item {
    Settings {
        id: settings
        settings: CoreInterface.settings  // or CoreInterface.globalSettings
        category: "DiffScope.CorePlugin.MyComponent"
        
        property alias windowWidth: root.width
        property alias windowHeight: root.height
        property alias isMaximized: root.visibility
    }
    
    Window {
        id: root
        width: 800
        height: 600
        
        // Properties are automatically synchronized with settings
        // through property aliases in the Settings component
    }
}
```

### Settings Component Properties

The `Settings` component requires:

- **settings**: A QSettings object (from `CoreInterface.settings` or `CoreInterface.globalSettings`)
- **category**: Optional group name (equivalent to C++ `beginGroup()`/`endGroup()`)

### Property Synchronization

Settings synchronization is achieved through property aliases:

```qml
Settings {
    id: settings
    settings: CoreInterface.settings
    category: "MyWindow"
    
    // These aliases automatically sync with QSettings
    property alias x: window.x
    property alias y: window.y
    property alias width: window.width
    property alias height: window.height
    property alias visible: window.visible
}

Window {
    id: window
    // Window properties are automatically restored from settings
}
```

## Best Practices

### 1. Use Appropriate Scope

- **User Settings**: Use `RuntimeInterface::settings()` for user preferences, window states, recent files, etc.
- **Global Settings**: Use `RuntimeInterface::globalSettings()` for system-wide configuration that affects all users

### 2. Group Organization

Always use groups to avoid key conflicts:

```cpp
// Good - organized by class/component
settings->beginGroup("HomeWindow");
settings->setValue("geometry", geometry);
settings->endGroup();

// Bad - no grouping, potential conflicts
settings->setValue("geometry", geometry);
```

### 3. Key Naming

Use descriptive, camelCase key names:

```cpp
// Good
settings->setValue("windowGeometry", geometry);
settings->setValue("isMaximized", maximized);
settings->setValue("recentProjects", projects);

// Bad
settings->setValue("geom", geometry);
settings->setValue("max", maximized);
settings->setValue("recent", projects);
```

### 4. Type Safety

Use appropriate QVariant types and provide defaults:

```cpp
// Reading with defaults
QRect geometry = settings->value("windowGeometry", QRect(100, 100, 800, 600)).toRect();
bool maximized = settings->value("isMaximized", false).toBool();
QStringList recent = settings->value("recentProjects", QStringList()).toStringList();

// Writing
settings->setValue("windowGeometry", window->geometry());
settings->setValue("isMaximized", window->isMaximized());
```

### 5. Settings Persistence Timing

- **Window State**: Save on window close or geometry change
- **User Preferences**: Save immediately when changed
- **Application State**: Save on application shutdown

```cpp
// Example: Save window state on geometry change
void MyWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    saveGeometry();
}

void MyWindow::saveGeometry() {
    auto settings = Core::RuntimeInterface::settings();
    settings->beginGroup("MyWindow");
    settings->setValue("geometry", geometry());
    settings->setValue("isMaximized", isMaximized());
    settings->endGroup();
}
```

## Common Patterns

### Window State Management

```cpp
class MyWindow : public QMainWindow {
private:
    void saveSettings() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup("MyWindow");
        settings->setValue("geometry", saveGeometry());
        settings->setValue("windowState", saveState());
        settings->endGroup();
    }
    
    void restoreSettings() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup("MyWindow");
        restoreGeometry(settings->value("geometry").toByteArray());
        restoreState(settings->value("windowState").toByteArray());
        settings->endGroup();
    }
};
```

### QML Window State

```qml
import SVSCraft.Extras

ApplicationWindow {
    id: window
    
    Settings {
        settings: CoreInterface.settings
        category: "MainWindow"
        
        property alias x: window.x
        property alias y: window.y
        property alias width: window.width
        property alias height: window.height
        property alias visibility: window.visibility
    }
}
```

### Plugin Configuration

```cpp
class MyPlugin : public ExtensionSystem::IPlugin {
private:
    void loadSettings() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup("MyPlugin");
        m_enabled = settings->value("enabled", true).toBool();
        m_configPath = settings->value("configPath", QString()).toString();
        settings->endGroup();
    }
    
    void saveSettings() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup("MyPlugin");
        settings->setValue("enabled", m_enabled);
        settings->setValue("configPath", m_configPath);
        settings->endGroup();
    }
};
```

This convention ensures consistent settings organization across the entire DiffScope project and provides a clear pattern for both C++ and QML components to persist their state and configuration.
