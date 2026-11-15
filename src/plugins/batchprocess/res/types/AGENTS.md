<!--
SPDX-FileCopyrightText: Team OpenVPI
SPDX-License-Identifier: GFDL-1.3-or-later
-->

# DiffScope Batch Processing Scripts

For scripts written entirely in JavaScript, the shipped TypeScript declarations and [installed examples](#installed-examples) provide the reference material. Start with [`runtime.d.ts`](runtime.d.ts) for script definitions and [`core.d.ts`](core.d.ts) for access to the current project, document, and selection. The other declarations describe the modules by their `diffscope:` names.

## Script Location and Layout

In DiffScope's settings, open **Batch Process** and read the **Script directory** field. This is the directory DiffScope scans for user scripts. The default is `Scripts` under the application's user configuration directory. Use the path shown in settings for the current installation.

DiffScope scans only the immediate children of the script directory. Each `.js` file is a standalone entry point. Each directory containing `index.js` is a script package. Use a package for helper modules or a QML panel. A package can also keep translation files alongside its entry point.

```text
<Script directory>/
    example.js
    example-package/
        index.js
        helpers.js
        i18n/
            en.json
```

Keep helper files inside their package so the loader discovers only the package entry point.

## Entry Points and Module Loading

DiffScope loads UTF-8 JavaScript in its embedded Qt JavaScript engine. Use `require()` with the module names in the declarations, such as `require("diffscope:core")`. Bare specifiers resolve to modules registered by DiffScope. Export the exact object returned by `defineScript()` through `module.exports` or `module.exports.default`.

For local dependencies, use paths such as `require("./helpers.js")` or `require("./defaults.json")` relative to the file containing the call. With an omitted extension, the loader tries `.js` and then `.json`. A package can load local modules only from within its own directory. A standalone script can load registered modules, and its local module scope is limited to its entry file.

Entry code runs during startup and reload. Keep application operations such as reading a project, opening a dialog, or editing the document inside handlers. Use a stable script ID unique across built-in and user scripts. The loader registers built-in scripts before user scripts, so a copy of a built-in example needs a new ID.

## First Script

Save this example as `example.js` at the root of the configured script directory.

```javascript
const { dialogs } = require("diffscope:shell");

module.exports = defineScript({
    id: "org.example.diffscope.batch-process",
    name: "Batch Process Example",
    version: "1.0.0",
    description: "Displays a confirmation message.",
    actions: [
        {
            name: "Show Example Message",
            description: "Confirms that the example script loaded.",
            execute() {
                dialogs.alert("The example script is running.");
            }
        }
    ]
});
```

Choose **Batch Process → Reload Scripts**, then **Batch Process → Show Example Message**. Reload after editing script files. A successful reload recreates the JavaScript engine and resets module state. For load errors or messages written with `console.log()`, open **Help → JavaScript Debug Console**.

## Project Actions

Set `requiresProject: true` for actions that operate on a project. These actions appear in project windows. Read `application.project` inside `execute()` to obtain the project associated with the invoking window. For document edits, collect input and validate the selection before starting a write transaction. Commit related changes together as an undo step. Save the project separately if the task requires it.

## QML Panels

DiffScope runs panel QML and any JavaScript written inside it in the application's QML engine. Batch processing `.js` files run in a separate dedicated JavaScript engine that provides the `diffscope:` script modules. Use the application's QML modules and APIs in panel code. Consult the application source for the QML types and interfaces that match the installed DiffScope version. [DiffScope source repository](https://github.com/diffscope/diffscope-project)

To run batch processing code from a panel, declare a method in the script's `methods` array and call it through the panel's `scriptContext.invokeMethod(name, params)`. Script panels are available under **View → All Panels** in a project window.

## Installed Examples

- [`transpose/index.js`](../builtin/transpose/index.js) demonstrates a project action that reads the selection, prompts for input, and commits one undoable edit. Its `i18n/` directory contains the script's translations.
- [`lrc-labels/index.js`](../builtin/lrc-labels/index.js) demonstrates file import and export, and [`KaraokeLyricsPanel.qml`](../builtin/lrc-labels/KaraokeLyricsPanel.qml) demonstrates calling script methods from a QML panel.
