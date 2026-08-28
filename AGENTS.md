<!--
SPDX-FileCopyrightText: Team OpenVPI
SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Repository Guidelines

## Project Architecture and Layout

DiffScope is a C++20/Qt 6 singing-voice editor. Keep `src/app` a thin launcher; product behavior belongs in application libraries or ChorusKit plugins.

- `src/app/`: executable entry point, application metadata, icons, and platform configuration.
- `src/libs/application/`: reusable first-party libraries. `loadapi` handles startup hooks, `uishell` provides the shared Qt Quick shell, and `transactional` provides undoable transaction support.
- `src/plugins/<plugin>/`: built-in plugins such as `coreplugin`, `audio`, `visualeditor`, and `synth`. A normal plugin contains `CMakeLists.txt`, `plugin.json.in`, C++ implementation, `qml/`, and `res/` (actions, icons, and translations).
- `src/plugins/coreplugin/`: the public application-level API and common project, window, singer, notification, settings, and editing services.
- `src/share/`: shared install/deployment rules used after ChorusKit finishes configuring the application.
- `src/libs/3rdparty/`: Git submodules. Treat these as upstream projects; do not mix ordinary feature changes with submodule edits.
- `src/libs/application/uishell/tests/`: standalone visual/smoke applications. `src/tests/` is reserved for application-wide tests.
- `scripts/vcpkg-manifest/` and `scripts/vcpkg/`: dependency manifest and overlay ports/triplets. `scripts/ci/` is the packaging reference. `dist/` contains installers.

Never commit generated files from `build/`, `cmake-build-*`, `out/`, `.qm`, MOC, QML cache, or synchronized include directories.

## Plugin and Dependency Boundaries

Extend the closest existing plugin before creating a new one. New built-in plugins must use `diffscope_add_builtin_plugin`, declare `project(<lowercase-target>)`, provide `plugin.json.in`, add their subdirectory in `src/plugins/CMakeLists.txt`, and keep CMake `LINKS` consistent with metadata `Dependencies`. Plugin IDs use reverse DNS (`org.diffscope.audio`); QML URIs use `DiffScope.<PascalCaseName>`.

Plugins derive from `ExtensionSystem::IPlugin`. Create self-contained services in `initialize()`, perform cross-plugin wiring only in `extensionsInitialized()`, and reserve `delayedInitialize()` for deferrable work. Per-window behavior belongs in a `WindowInterfaceAddOn`; register shared/per-window objects in the appropriate ChorusKit object pool and retrieve them through typed APIs such as `getFirstObject<T>()`. Do not reach into another plugin's source `internal/` tree.

## Reuse the Existing Infrastructure

Do not build parallel service, action, theming, document, or audio frameworks.

- **ChorusKit / ExtensionSystem / CoreApi:** use the plugin lifecycle, `RuntimeInterface` (QML engine, user/global settings, application services), `CoreInterfaceBase`, window interfaces, registries, and object pools.
- **QActionKit:** declare action IDs and menu/toolbar insertions in `res/org.diffscope.<plugin>_actions.xml`. Bind behavior with QML `ActionCollection`/`ActionItem`, then register it with the window's `actionContext`. Use `Core::CoreInterface::actionRegistry()` for global action metadata and layouts. Do not hand-create duplicate shortcuts or menus.
- **SVSCraft:** use its QML controls, `Theme`, `ThemedItem`, dialogs, icons, settings helpers, and music time/timeline types. New UI should visually and behaviorally match these components rather than introducing local substitutes. Avoid hardcoding color values and use colors defined in `Theme`.
- **Core plugin:** use `CoreInterface` for application commands and registries; `HomeWindowInterface`/`ProjectWindowInterface` for window-scoped behavior; `ProjectDocumentContext` for open/save; `DspxDocument` for model, selection, clipboard, and `TransactionController`; `ProjectTimeline` for playhead/time mapping; and existing scenarios, notifications, singer registries, default-lyric services, and property-editor hooks.
- **Domain libraries:** use `dspxmodel*` for the live document/selection model, OpenDSPX for serialization/interchange, ScopicFlow for editor view models and interaction controllers, and TALCS/audio-plugin contexts for audio. Keep UI coordinates, document ticks, and audio samples in their owning layers and convert at boundaries.

## Data Compatibility and Versioning

Unless explicitly requested, do not account for compatibility with existing persisted data or interchange data formats. When changing such data or formats, do not update their header metadata or version numbers.

## Feature Replacement Hygiene

When asked to modify a feature, treat the requested result as the only current behavior. Unless the user explicitly requests otherwise, do not leave any remnants of the old behavior in code, comments, or documentation. Remove obsolete implementations, branches, APIs, settings, names, examples, compatibility paths, migration notes, comparisons, historical context, and explanations of why the feature was changed.

## Header Publication and Include Paths

CMake's `ck_sync_include` publishes headers into the build include tree and deliberately hides most source-directory nesting. Include the published path, never `src/...` or a relative path across directories:

| Source example | Consumer include |
| --- | --- |
| `src/plugins/coreplugin/project/document/DspxDocument.h` | `<coreplugin/DspxDocument.h>` |
| `src/plugins/audio/audiocontext/ProjectAudioContext_p.h` | `<audio/private/ProjectAudioContext_p.h>` |
| `src/plugins/audio/internal/addon/PlaybackAddOn.h` | `<audio/internal/PlaybackAddOn.h>` |
| `src/plugins/maintenance/internal/ApplicationUpdateChecker_p.h` | `<maintenance/internal/private/ApplicationUpdateChecker_p.h>` |
| `src/libs/application/uishell/src/BubbleNotificationHandle.h` | `<uishell/BubbleNotificationHandle.h>` |

Public and private headers are flattened to `<target>/` and `<target>/private/`; implementation headers are flattened to `<target>/internal/` (or its `private/` child). Consequently, header basenames must be unique within each publication class of a target. An `internal` header is build-private and must not become a cross-plugin API merely because it has a synchronized path.

## Include Format and Ordering

In every header, use angle brackets for **all** includes, including first-party headers. In a `.cpp`, the matching header is the first include and the only header included with quotes. Every other header—including the PImpl header and same-plugin headers—uses its synchronized angle-bracket path. Generated `moc_*.cpp` inclusions at the end of a source file are not headers and remain quoted.

Separate include groups with one blank line and let `.clang-format` sort each group case-sensitively:

1. Matching header (`.cpp` only).
2. Platform/C and C++ standard library headers, then Boost.
3. Qt public headers, Qt QPA headers, then Qt private headers.
4. Frameworks in the configured order: `CoreApi`, ExtensionSystem, OpenDSPX, QActionKit, QWindowKit, SVSCraft, ScopicFlow, then TALCS.
5. Application layers: `dspxmodel*`, `loadapi`, `uishell`, `coreplugin`, then the current plugin and otherwise uncategorized headers.

```cpp
#include "AudioExporter.h"

#include <algorithm>
#include <memory>

#include <QFile>
#include <QString>

#include <CoreApi/runtimeinterface.h>
#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <audio/private/AudioExporter_p.h>
```

Include what the file uses and prefer forward declarations in headers when a complete type is unnecessary.

## C++ Style and Naming

`.clang-format` is authoritative: four spaces, no tabs, attached braces, right-aligned pointers/references, LF endings, and no column limit. Format changed C/C++/Objective-C++ files with `clang-format -i <files>`.

- Use `PascalCase.h/.cpp` matching the primary type. Module-global headers are the established lowercase exception, for example `coreglobal.h`.
- Use `PascalCase` types and QML components, `lowerCamelCase` functions/properties/locals, and `m_lowerCamelCase` direct data members. PImpl members have no `m_` prefix.
- Match established namespace and acronym spelling (`Core`, `Audio::Internal`, `Dspx`, `MIDI`) instead of inventing variants.
- Use `#ifndef` guards such as `DIFFSCOPE_AUDIO_AUDIOEXPORTER_H`; do not introduce `#pragma once`.
- Use the target export macro on public ABI classes (`CORE_EXPORT`, `AUDIO_EXPORT`, and so on).
- Use full Qt forms: `Q_SIGNALS`, `Q_SLOTS`, and `Q_EMIT`, not `signals`, `slots`, or bare signal calls when emitting.
- Make single-argument constructors `explicit`, mark overrides, initialize pointers, and use QObject parent ownership where appropriate.
- Prefer C++20 ranges and `<=>` where they make code clearer. Do not use exceptions for normal control flow; catch only at APIs that can throw, translate the failure there, and preserve useful diagnostics.

## Class Design and PImpl

Exported QObject-based interfaces and other stable public APIs use Qt-style PImpl. Keep `Class.h`, `Class.cpp`, and `Class_p.h` together in the source tree; consumers see the private header as `<module/private/Class_p.h>`. The public class forward-declares `ClassPrivate`, uses `Q_DECLARE_PRIVATE(Class)`, owns `QScopedPointer<ClassPrivate> d_ptr`, and has an out-of-line destructor. The private class uses `Q_DECLARE_PUBLIC(Class)` and stores `Class *q_ptr`. Use `Q_D`/`Q_Q` in implementations. Data/value types may use `QSharedDataPointer` when implicit sharing is intended.

```cpp
// Widget.h
class MODULE_EXPORT Widget : public QObject {
    Q_OBJECT
    Q_DECLARE_PRIVATE(Widget)
public:
    explicit Widget(QObject *parent = nullptr);
    ~Widget() override;
private:
    QScopedPointer<WidgetPrivate> d_ptr;
};

// Widget.cpp
#include "Widget.h"

#include <module/private/Widget_p.h>
```

Internal classes under `internal/` do not need PImpl; store their members directly with `m_` names. C++ types meant only for QML may use `Type_p.h` as their primary header; if such a type needs a private implementation/attached helper, use `Type_p_p.h`. QML property notify signals are normally parameterless; regular C++ API signals may carry the new value. Keep declarations ordered as Qt macros/properties, public API, signals/slots, protected API, then private storage.

Prefer ChorusKit registries/object pools over adding a singleton. If lifecycle ownership requires one, assert that no instance exists in the constructor, assign the static instance there, and clear it in the destructor; these objects are main-thread services. Restrict construction with a private/protected constructor and plugin friend where appropriate. A QML singleton additionally uses `QML_SINGLETON` and a static `create(QQmlEngine *, QJSEngine *)` factory.

## QML, Actions, Settings, and Accessibility

Use four-space QML indentation. Name files/types `PascalCase.qml`, IDs and properties `lowerCamelCase`, and declare injected dependencies as `required property`. Group imports as Qt, QActionKit/other framework modules, SVSCraft, then DiffScope modules. Prefer layouts and anchors over fixed coordinates.

All user-visible C++ strings use `tr()` and QML strings use `qsTr()`. Action text belongs in the action XML so QActionKit can translate and reuse it. Commit source `.ts` updates under the owning module's `res/translations/`; `.qm` files are generated. After changing translatable text, run:

```sh
cmake --build build --target <plugin>_translations_lupdate
cmake --build build --target <plugin>_translations_lrelease
```

Write user-facing window, dialog, and page titles, button text, and menu text in Title Case. Use sentence case for all other general user-facing text. Format numbers for the user's locale unless they are special identifiers such as HTTP status codes or require a defined representation such as timecodes. Use localized placeholders such as `%L1` and `%Ln`, `QLocale::toString()` in C++, or `toLocaleString()` in JavaScript/QML instead of inserting locale-neutral numeric strings. Describe concepts, actions, and results from the user's business perspective; do not over-explain internal implementation details in user-facing copy.

Use `RuntimeInterface::settings()` for user preferences and `RuntimeInterface::globalSettings()` only for machine-wide state. Group C++ keys by `staticMetaObject.className()` (or an equally stable module name), use descriptive `lowerCamelCase` keys, supply defaults, and balance every `beginGroup()` with `endGroup()`. In QML, use `SVSCraft.Extras.Settings` with a stable module-qualified category.

Every interactive control must be keyboard reachable. Give icon-only controls meaningful `text`, tooltips, and `Accessible` metadata; preserve logical focus order. Use `control.mirrored`, layouts, and left/right padding correctly for RTL interfaces. Test long translations and never encode meaning by color alone.

## Document Editing and View-Model Binding

Visualized editors must not mutate the DSPX document directly. Route gestures through interaction controllers or core scenarios. Every user edit must begin a `TransactionController` transaction, commit with a concise translated description only when data changed, and abort on cancellation, startup failure, or no-op.

View-model bindings maintain explicit document-to-view and view-to-document maps. Bind/unbind on collection signals, guard equality to avoid feedback loops, and allow view-to-document writes only in the corresponding active edit/move state; otherwise restore the document value. Synchronize selection explicitly. Complex gestures use named pending/progressing/committing/aborting states and log every state entry/exit with a per-context `QLoggingCategory`.

## Configure, Build, and Run

Use a recursive checkout, CMake 3.19+, Qt 6.10-compatible development packages, Ninja, and a C++20 compiler. During vcpkg installation, expose the Qt CMake directory through `QT_DIR`/`Qt6_DIR` and retain those variables for overlay ports.

Do not perform any test, build, run, or automatic formatting operations unless explicitly requested by the user. Also do not include any of these operations in the plan unless explicitly requested by the user.

```sh
git submodule update --init --recursive
<vcpkg>/vcpkg install --x-manifest-root=scripts/vcpkg-manifest --x-install-root=<vcpkg>/installed --triplet=<triplet>
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake \
  -DQt6_DIR=<qt>/lib/cmake/Qt6 -DAPPLICATION_BUILD_TESTS=ON \
  -DTALCS_DSPX=ON -DTALCS_WIDGETS=ON
cmake --build build --parallel
./build/out/bin/DiffScope
cmake --install build --prefix <install-dir>
```

On Windows, run these commands in a compiler developer shell and use `DiffScope.exe`. Build one target with `cmake --build build --target <target>`. For CI/package parity, use `scripts/ci/Build.ps1`; it performs a RelWithDebInfo configure, full build, and install and requires vcpkg and the `dspm` executable paths.

Useful options include `APPLICATION_INSTALL`, `APPLICATION_ENABLE_DEVEL`, `APPLICATION_ENABLE_BREAKPAD`, and `APPLICATION_CONFIGURE_INSTALLER`. Keep machine-specific paths in CMake cache entries or environment variables, never in source files.

## Testing and Validation

`APPLICATION_BUILD_TESTS=ON` builds the `tst_uishell_<Feature>` visual applications under `build/out/bin`; launch the affected one and exercise the changed interaction. These are manual GUI smoke tests and are not currently registered with CTest. For new deterministic logic tests, prefer Qt Test, name the target `tst_<module>_<Feature>`, register it with `add_test`, and run:

```sh
ctest --test-dir build --output-on-failure
cmake --build build --target <qml-target>_qmllint
```

There is no numeric coverage gate. Cover new branches and regressions proportionally, and report exact automated and manual checks. UI changes require screenshots or a short capture, plus checks for keyboard operation, theme variants, scaling, and representative locales. Do not count third-party submodule tests as validation of first-party behavior.

## Commits and Pull Requests

Recent history uses short, imperative, sentence-case subjects such as `Add synth plugin`, `Fix bugs with language settings`, and `Update audio export extension name logic`. Follow that style; keep each commit focused and avoid a mandatory Conventional Commit prefix.

A pull request must explain the user-visible result, affected modules, architecture/API choices, compatibility or migration concerns, and the commands/tests run. Link relevant issues. Include before/after visuals for QML or layout changes. Call out action IDs, settings keys, translation changes, dependency/license changes, and submodule pointer bumps explicitly. Keep unrelated formatting, generated output, and local configuration out of the diff.

## Security and Configuration

Never commit credentials, tokens, private service URLs, machine paths, user projects, or generated package-manager state. Validate paths and external-process arguments at boundaries, keep log messages useful without exposing sensitive content, and use Qt's native path and process APIs. Preserve each plugin's declared license and review the license impact before adding or updating a dependency.
