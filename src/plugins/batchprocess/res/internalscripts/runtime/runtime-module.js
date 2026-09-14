// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(bridge => {
    "use strict";

    const freeze = Object.freeze;

    function currentRuntime() {
        const runtime = bridge.currentScriptRuntime();
        const actions = runtime.metadata.actions;
        for (let index = 0; index < actions.length; ++index) {
            freeze(actions[index]);
        }
        freeze(actions);
        freeze(runtime.metadata);
        return freeze(runtime);
    }

    const script = {};
    for (const property of ["metadata", "scriptPath", "scriptRoot", "locale", "platform", "kind", "action", "actionIndex"]) {
        Object.defineProperty(script, property, {
            configurable: false,
            enumerable: true,
            get() {
                return currentRuntime()[property];
            },
        });
    }
    freeze(script);

    function abort() {
        bridge.abort();
        throw new Error("Script execution interruption did not stop execution.");
    }

    return freeze({
        script,
        abort,
    });
})
