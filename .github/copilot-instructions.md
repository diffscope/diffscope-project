# DiffScope Development Guidelines

## Architecture Overview

DiffScope is a modern Qt-based music production application built on a sophisticated plugin architecture derived from Qt Creator. The project uses **ChorusKit** as its plugin framework foundation.

### Core Architecture Layers
- **Application Layer**: Main executable (`src/app/main.cpp`) with Qt/QML integration
- **Plugin System**: ChorusKit-based extensible architecture (`src/plugins/`)
- **UI Framework**: SVSCraft QML components for modern interface design
- **Action System**: QActionKit for declarative menu/toolbar definitions

## Essential Framework Knowledge

### 1. Plugin System (ChorusKit)
- Plugins are the primary extension mechanism, following Qt Creator patterns
- Each plugin has `plugin.json.in` metadata and extends `ExtensionSystem::IPlugin`
- Core plugin (`coreplugin`) provides fundamental services via `CoreInterface` singleton
- Plugin communication through well-defined interfaces, not direct linking
- **Plugin changes require restart** - enabling/disabling plugins only takes effect after application restart

```cpp
// Plugin registration pattern
class CorePlugin : public ExtensionSystem::IPlugin {
    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
};
```

### 2. QML Integration Patterns
- Heavy use of Qt Quick for UI with C++ backend services
- Custom QML modules: `DiffScope.CorePlugin`, `DiffScope.UIShell`, `SVSCraft.UIComponents`
- QML singletons expose C++ services (e.g., `CoreInterface` as QML singleton)
- Theme system integrated via `SVSCraft.UIComponents`

### 3. Action System (QActionKit)
- Declarative action definitions in XML (`res/core_actions.xml`)
- Actions auto-generate C++ code via `qak_add_action_extension()`
- Supports complex menu hierarchies with shortcuts, translations, and context

## Build System Specifics

### CMake Configuration
```bash
# Standard development build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DAPPLICATION_BUILD_TESTS=ON
cmake --build build
```

### Key CMake Functions
- `ck_add_plugin()`: Register ChorusKit plugins with metadata
- `qm_configure_target()`: Enhanced target configuration with Qt integration
- `svs_add_library()`: SVSCraft library configuration
- `qak_add_action_extension()`: Generate action code from XML

### Testing
- Enable with `-DAPPLICATION_BUILD_TESTS=ON`
- Individual test executables in `src/libs/*/tests/` and `src/tests/`
- QML component tests use standalone applications for visual testing

## Development Workflows

### Adding New Plugins
1. Create plugin directory in `src/plugins/[name]/`
2. Add `CMakeLists.txt` with `ck_add_plugin()` call
3. Create `plugin.json.in` metadata file
4. Implement plugin class extending `ExtensionSystem::IPlugin`
5. Export QML modules if UI components needed

### QML Component Development
- Follow SVSCraft theming patterns: `Theme.backgroundColor`, `ThemedItem.backgroundLevel`
- Use `qt_add_qml_module()` for proper QML module registration
- Test components with standalone test applications (see `tests/` directories)

### Action System Usage
```xml
<!-- res/actions.xml -->
<action id="my.action" text="&amp;Action" class="File" shortcut="Ctrl+M" />
```

## Project-Specific Conventions

### Naming Patterns
- Plugin IDs: `org.OpenVPI.DiffScope.Plugin`
- QML URIs: `DiffScope.[PluginName]`, `SVSCraft.[Component]`
- CMake targets: Match directory names, use `${PROJECT_NAME}` consistently

### Code Organization
- Public headers in plugin root or `core/` subdirectories
- Private implementation in `internal/` subdirectories  
- QML files in `qml/` subdirectories with URI structure
- Resources in `res/` with organized icon/theme structure

### Dependencies
- **ChorusKit**: Plugin framework and app lifecycle
- **SVSCraft**: UI components, theming, utilities (LGPL v3)
- **QActionKit**: Declarative action/menu system
- **qmsetup**: CMake utilities and build configuration
- Qt 6.8+ with Core, Gui, Qml, Quick modules required

## Code Conventions

### C++ Code
- Do not use try-catch, unless it is an error handling for an external library.
- Use C++20 ranges features where applicable for better readability and performance.
- Use C++20 `<=>` operator for three-way comparisons.

### C++ Class Design
Refer to [C++ Class Conventions](instructions/class-conventions.instructions.md) for detailed guidelines on:
- External interface classes with private implementation (pimpl pattern)
- Internal implementation classes without pimpl
- QML element classes with `_p.h` suffix  
- QML attached type classes with `_p_p.h` suffix
- Include guard formats, file naming, and code formatting standards

### Settings Management
Refer to [Settings Conventions](instructions/settings-conventions.instructions.md) for QSettings usage patterns:
- User-scoped vs global settings access via `RuntimeInterface`
- Group naming conventions for C++ classes and QML components
- QML settings integration through `SVSCraft.Extras.Settings`
- Property synchronization and persistence timing best practices

### Accessibility Design
Refer to [Accessibility Conventions](instructions/accessible-conventions.instructions.md) for comprehensive accessibility guidelines:
- Screen reader and assistive technology compatibility
- Focus management and keyboard navigation
- Accessible properties for custom QML controls (`Accessible.role`, `Accessible.name`, `Accessible.description`)
- Internationalization support including right-to-left layouts and locale-aware formatting
- Use of `DescriptiveText` component for enhanced accessibility descriptions

## Critical Integration Points

### Plugin Initialization Order
```cpp
// Standard plugin lifecycle
bool initialize() override; // Setup core services
void extensionsInitialized() override; // Cross-plugin communication ready
```

### Plugin State Management
- Plugin enable/disable changes tracked via `PluginSpecHelper::isRestartRequired()`
- Restart triggered via `CoreInterfaceBase::restartApplication()` which sets `qApp->property("restart", true)`
- Loader's `Restarter` class handles application restart with `QProcess::startDetached()`
- Settings persisted immediately but plugins only loaded/unloaded on restart

### Window Management
- `ProjectWindowInterface` for main application windows
- `ProjectWindowWorkspaceLayout` for flexible panel positioning
- Panel positions: `LeftTop`, `RightBottom`, etc.

### QML-C++ Bridge
- Use `Q_INVOKABLE` for C++ methods called from QML
- Register types with `qmlRegisterType<>()`
- Prefer QML singletons for service access: `CoreInterface.actionRegistry`

This architecture enables plugins, declarative UI definitions, and maintainable separation between core functionality and extensions.