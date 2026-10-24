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
- `scripts/vcpkg-manifest/`: dependency manifest. `scripts/vcpkg-sources.json` specifies the commits of vcpkg and of the overlays, which `scripts/vcpkg-workspace.cmake` checks out into the ignored directory `deps/`. `scripts/ci/` is the packaging reference. `dist/` contains installers.

Never commit generated files from `build/`, `cmake-build-*`, `out/`, `.qm`, MOC, QML cache, or synchronized include directories.

## Document Creation

Do not create any documents inside the project directory unless explicitly requested by the user. If documentation is necessary without such a request, create it outside the project directory and inform the user of its location.

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

In every header, use angle brackets for **all** includes, including first-party headers. In a `.cpp`/`.mm`, the matching public header and, when present, its private `_p.h` partner are the only quoted includes, written as plain relative filenames (`#include "Class.h"` then `#include "Class_p.h"`, public before private). In a `Class_p.h`, the corresponding public header must be the first include, alone in its own standalone group. Every other header—including same-directory siblings and same-plugin headers—uses its synchronized angle-bracket path. Generated `moc_*.cpp` inclusions at the end of a source file are not headers and remain quoted.

Platform-split implementation files that share one class header (for example `Class_icu.cpp` and `Class_NSString.mm` both including `Class.h`) treat that shared header as their matching header. Generated headers outside the synchronized include tree, such as protobuf `*.qpb.h` outputs, are exempt from the synchronized-path rule and may remain quoted.

Separate include groups with one blank line and let `.clang-format` sort each group case-sensitively:

1. Matching headers: the file's public header, then its `_p.h` private header (`.cpp`/`.mm` only). In a `_p.h`, the corresponding public header takes this place.
2. Platform/C and C++ standard library headers, then Boost.
3. Qt public headers, Qt QPA headers, then Qt private headers.
4. Frameworks in the configured order: `CoreApi`, ExtensionSystem, OpenDSPX, QActionKit, QWindowKit, SVSCraft, ScopicFlow, then TALCS.
5. Application layers: `dspxmodel*`, `loadapi`, `uishell`, `coreplugin`, then the current plugin and otherwise uncategorized headers.

```cpp
#include "AudioExporter.h"
#include "AudioExporter_p.h"

#include <algorithm>
#include <memory>

#include <QFile>
#include <QString>

#include <CoreApi/runtimeinterface.h>
#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ProjectWindowInterface.h>
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
- Unless explicitly required, never declare any classes as `final`.

## Fixed QML Component Failure Handling

Treat a QML component identified by a fixed module URI and type name as a required application component unless the surrounding code or the user's requirement provides explicit evidence of fallback behavior.

```cpp
QQmlComponent component(engine, "Module.Package", "ComponentName");
if (component.isError()) {
    qFatal() << component.errorString();
}

QObject *object = component.createWithInitialProperties(initialProperties);
if (!object) {
    qFatal() << component.errorString();
}

ExpectedClass *typedObject = qobject_cast<ExpectedClass *>(object);
if (!typedObject) {
    qFatal("ComponentName in Module.Package is not ExpectedClass");
}
```

- Check `isError()` immediately after constructing a `QQmlComponent` from a fixed module URI and type name. Report the failure only with `qFatal() << component.errorString()`. Do not add prefixes, developer guidance, user-facing dialogs, or alternate diagnostics.
- If the same code path immediately calls `create()` or `createWithInitialProperties()`, retain the raw `QObject *` and check it before dereferencing it, assigning a parent, reading a property, or calling a method. Report a null result only with `qFatal() << component.errorString()`.
- If the created object requires a specific C++ type, perform the null check before `qobject_cast`. Report a failed cast with the exact message `ComponentName in Module.Package is not ExpectedClass`, using the actual QML component name, module URI, and C++ class name.
- `Q_ASSERT`, `qWarning`, `qCritical`, user-facing messages, and null returns do not replace these fatal checks.
- The load check applies even if the component is subsequently passed to another object. Creation and type checks apply only when creation occurs immediately in the same code path. Do not trace components through helper parameters, members, registries, action contexts, or other indirect creation paths.
- Preserve an existing nonfatal path only if the code explicitly implements fallback behavior for that component.

## Prose Style

These rules apply to comments, documentation, README files, help text, and diagnostic messages.

- **Use formal technical writing.** Comments and documentation are normative text, not narration. Each sentence states a fact, a constraint, or a reason, without preamble, impressions, or rhetoric.
- **Do not personify.** Code, files, formats, programs, and tests are not subjects with a will. Do not write says, tells, knows, asks, wants, means, cares, decides, promises, or is told. Use returns, indicates, records, specifies, reports, detects, requires, or rejects instead. Real actors such as the user, the author, or the caller may be subjects.
- **Use terms, not descriptive paraphrases.** Write invalid byte sequence, not bytes that do not decode. Write unpaired surrogate, reverse mapping, and unrepresentable character, not half of one, the way back, and what cannot be spelled. If no common term exists, define one at its first use and keep that name throughout.
- **Headings, group names, and list labels are nouns or noun phrases.** Write Motivation, Behavior, and Rationale, not Why, What it does, and How it works. If a noun works, do not use a what-clause as a subject or object. Write the requested encoding, not what the user asked for.
- **Use if for conditions and where only for places.** Write empty if absent, not empty where there is none. Do not use one to refer back to a noun (such a one, the one it wants). Repeat the noun.
- **Avoid colloquial phrases** such as whatever else, for good, as it stands, on its own, on the way out, at a glance, there and back, is given up on, the rest of why, and and all. Use precise written expressions instead.
- **Write complete sentences.** Do not write fragments, comma splices (two independent clauses joined only by a comma), or rhetorical questions. Do not start a causal statement with So, And so, Which is why, That is why, or Hence. State the cause in the same sentence with because or therefore. Do not address the reader in the second person.
- **Do not use dashes, and do not join clauses with semicolons.** Start a new sentence wherever a sentence break belongs. Use American spelling.
- **Function documentation starts with a verb** (Returns, Decodes, Reads, Rejects). State the return value for each case. Boolean queries read Returns whether ….
- **Do not write a paragraph for what a few words can say.** Comments explain constraints, ownership, lifetimes, and the reason the implementation must take its current form. Do not restate what the signature already shows.
- **Do not write archaeological comments.** Keep the reason the code must be this way now. Remove how it used to be and how it was fixed.
- The same rules apply to Chinese text. Use written language, not colloquial words such as 「别」「搞」「就行」「得（表必须）」「啥」「拿来」「反正」「其实」「说白了」「这玩意儿」. Headings use 「动机」「设计理由」「实现方式」, not 「为什么」「怎么做」. Normative imperatives such as 「不要」「必须」 are not colloquial and remain in use.

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
#include "Widget_p.h"
```

Internal classes under `internal/` do not need PImpl; store their members directly with `m_` names. C++ types meant only for QML may use `Type_p.h` as their primary header; if such a type needs a private implementation/attached helper, use `Type_p_p.h`. QML property notify signals are normally parameterless; regular C++ API signals may carry the new value. Keep declarations ordered as Qt macros/properties, public API, signals/slots, protected API, then private storage.

Prefer ChorusKit registries/object pools over adding a singleton. If lifecycle ownership requires one, assert that no instance exists in the constructor, assign the static instance there, and clear it in the destructor; these objects are main-thread services. Restrict construction with a private/protected constructor and plugin friend where appropriate. A QML singleton additionally uses `QML_SINGLETON` and a static `create(QQmlEngine *, QJSEngine *)` factory.

## QML, Actions, Settings, and Accessibility

Use four-space QML indentation. Name files/types `PascalCase.qml`, IDs and properties `lowerCamelCase`, and declare injected dependencies as `required property`. Group imports as Qt, QActionKit/other framework modules, SVSCraft, then DiffScope modules. Prefer layouts and anchors over fixed coordinates.

Avoid redundant control property declarations. Determine the effective defaults from the Qt base type, the SVSCraft control, and any project component in the inheritance chain.

- Omit properties that repeat effective defaults unless a specific interaction requires an explicit declaration. Examples include `ToolButton.flat: true`, `SpinBox.editable: true`, `DoubleSpinBox.editable: true`, and `DescriptiveText.activated: hovered` on controls that already provide that binding.
- Layout settings, including padding, spacing, orientation, alignment, and font, may (but not must) explicitly repeat default values. Icon dimensions are an exception. Omit `icon.width` and `icon.height` if they match the effective defaults, including the SVSCraft default of 16.
- Set `ToolButton.focusPolicy` only if a specific interaction requires an explicit focus policy. Differences between platform defaults alone do not justify a declaration.
- Button types derived from `AbstractButton` obtain their accessible names from `text`. Do not repeat the same value in `Accessible.name`, including through an intermediate property or an equivalent expression. Provide an explicit name only if it adds distinct semantics or the control has no suitable text. Field labels and custom content require separate consideration.
- SVSCraft `Button` and `ToolButton` provide automatic action tooltips for nonempty `text` in `AbstractButton.IconOnly` mode. Do not duplicate these tooltips with `ToolTip.visible: hovered`, `ToolTip.text: text`, or an equivalent `DescriptiveText` configuration.
- Use `DescriptiveText` for additional tooltip, status, and context-help text. If the accessible description should use the tooltip and status text, explicitly enable `DescriptiveText.bindAccessibleDescription` instead of maintaining a duplicate `Accessible.description` binding. This binding is disabled by default. `DescriptiveText` does not supply the accessible name.
- Preserve custom tooltip content and activation conditions, including keyboard focus, disabled-state explanations, and text truncation. Ensure that custom tooltips do not duplicate the control's automatic action tooltip.

All user-visible C++ strings use `tr()` and QML strings use `qsTr()`. Action text belongs in the action XML so QActionKit can translate and reuse it. Commit source `.ts` updates under the owning module's `res/translations/`; `.qm` files are generated. After changing translatable text, run:

```sh
cmake --build build --target <plugin>_translations_lupdate
cmake --build build --target <plugin>_translations_lrelease
```

Write user-facing window, dialog, and page titles, button text, and menu text in Title Case. Use sentence case for all other general user-facing text. Format numbers for the user's locale unless they are special identifiers such as HTTP status codes or require a defined representation such as timecodes. Use localized placeholders such as `%L1` and `%Ln`, `QLocale::toString()` in C++, or `toLocaleString()` in JavaScript/QML instead of inserting locale-neutral numeric strings. Describe concepts, actions, and results from the user's business perspective; do not over-explain internal implementation details in user-facing copy.

Use `RuntimeInterface::settings()` for user preferences and `RuntimeInterface::globalSettings()` only for machine-wide state. Group C++ keys by `staticMetaObject.className()` (or an equally stable module name), use descriptive `lowerCamelCase` keys, supply defaults, and balance every `beginGroup()` with `endGroup()`. In QML, use `SVSCraft.Extras.Settings` with a stable module-qualified category.

Every interactive control must be keyboard reachable. Give icon-only controls meaningful `text` and use the control's tooltip and accessibility defaults unless additional semantics are required. Preserve logical focus order. Use `control.mirrored`, layouts, and left/right padding correctly for RTL interfaces. Test long translations and never encode meaning by color alone.

Unless explicitly required, do not fill in any translated strings in TS files.

## Document Editing and View-Model Binding

Visualized editors must not mutate the DSPX document directly. Route gestures through interaction controllers or core scenarios. Every user edit must begin a `TransactionController` transaction, commit with a concise translated description only when data changed, and abort on cancellation, startup failure, or no-op.

View-model bindings maintain explicit document-to-view and view-to-document maps. Bind/unbind on collection signals, guard equality to avoid feedback loops, and allow view-to-document writes only in the corresponding active edit/move state; otherwise restore the document value. Synchronize selection explicitly. Complex gestures use named pending/progressing/committing/aborting states and log every state entry/exit with a per-context `QLoggingCategory`.

## Configure, Build, and Run

Use a recursive checkout, CMake 3.19+, Qt 6.10-compatible development packages, Ninja, and a C++20 compiler. `scripts/vcpkg-workspace.cmake` prepares vcpkg, the overlays, and the dependencies in `deps/`, and passes the Qt CMake directory to the ports that build against Qt. `cmake -P scripts/vcpkg-workspace.cmake help` lists its commands, and `--dry-run` prints the commands without running them. The configuration requires `CMAKE_TOOLCHAIN_FILE`, the toolchain of `deps/vcpkg`.

Do not perform any test, build, run, or automatic formatting operations unless explicitly requested by the user. Also do not include any of these operations in the plan unless explicitly requested by the user.

```sh
git submodule update --init --recursive
cmake -P scripts/vcpkg-workspace.cmake install --qt-dir <qt>/lib/cmake/Qt6 --triplet <triplet>
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=deps/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DQt6_DIR=<qt>/lib/cmake/Qt6 -DAPPLICATION_BUILD_TESTS=ON \
  -DTALCS_DSPX=ON -DTALCS_WIDGETS=ON
cmake --build build --parallel
./build/out/bin/DiffScope
cmake --install build --prefix <install-dir>
```

On Windows, run these commands in a compiler developer shell and use `DiffScope.exe`. Build one target with `cmake --build build --target <target>`. For CI/package parity, use `scripts/ci/Build.ps1`. It performs a RelWithDebInfo configure, full build, and install and requires a vcpkg root directory. The optional `DsspDirectory` parameter specifies the bundled dssp installation directory.

Useful options include `APPLICATION_INSTALL`, `APPLICATION_ENABLE_DEVEL`, `APPLICATION_ENABLE_BREAKPAD`, and `APPLICATION_CONFIGURE_INSTALLER`. Keep machine-specific paths in CMake cache entries or environment variables, never in source files.

## Testing and Validation

`APPLICATION_BUILD_TESTS=ON` builds the `tst_uishell_<Feature>` visual applications under `build/out/bin`; launch the affected one and exercise the changed interaction. These are manual GUI smoke tests and are not currently registered with CTest. For new deterministic logic tests, prefer Qt Test, name the target `tst_<module>_<Feature>`, register it with `add_test`, and run:

```sh
ctest --test-dir build --output-on-failure
cmake --build build --target <qml-target>_qmllint
```

There is no numeric coverage gate. Cover new branches and regressions proportionally, and report exact automated and manual checks. UI changes require screenshots or a short capture, plus checks for keyboard operation, theme variants, scaling, and representative locales. Do not count third-party submodule tests as validation of first-party behavior.

## Commits and Pull Requests

- **Write the commit message as a single line**: an English, sentence-case imperative subject in American spelling, such as `Add synth plugin`, `Fix bugs with language settings`, or `Update audio export extension name logic`. Do not add a body or a mandatory Conventional Commit prefix. Design rationale belongs in code comments or the pull request description, not in the commit message.
- **Commit messages carry no AI attribution.** Do not add `Co-Authored-By` or `Generated with` lines.
- **Each commit does one thing.** Split work into small commits by concern instead of one large commit. If a file contains changes for two commits, take the old content with `git show HEAD:<path>`, apply only one set of changes, commit it, and then restore the complete version. Do not stage the whole file for convenience.
- Every commit must build and pass its tests on its own, including the intermediate states created by splitting.
- **Do not commit without the author's authorization, and above all do not push without it.** Leave the changes in the working tree until the author approves the commit.

A pull request must explain the user-visible result, affected modules, architecture/API choices, compatibility or migration concerns, and the commands/tests run. Link relevant issues. Include before/after visuals for QML or layout changes. Call out action IDs, settings keys, translation changes, dependency/license changes, and submodule pointer bumps explicitly. Keep unrelated formatting, generated output, and local configuration out of the diff.

## Security and Configuration

Never commit credentials, tokens, private service URLs, machine paths, user projects, or generated package-manager state. Validate paths and external-process arguments at boundaries, keep log messages useful without exposing sensitive content, and use Qt's native path and process APIs. Preserve each plugin's declared license and review the license impact before adding or updating a dependency.
