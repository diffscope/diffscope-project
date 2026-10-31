// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(bridge => {
    "use strict";

    const freeze = Object.freeze;

    class MethodLogicError extends Error {
        constructor(message, data) {
            super(message);
            this.name = "MethodLogicError";
            this.data = data;
        }
    }

    function freezeTree(value) {
        if (value === null || typeof value !== "object" || Object.isFrozen(value)) {
            return value;
        }
        freeze(value);
        for (const key of Object.keys(value)) {
            freezeTree(value[key]);
        }
        return value;
    }

    const environment = freeze({
        get locale() {
            return bridge.currentLocale();
        },
        get platform() {
            return bridge.currentPlatform();
        },
    });

    function currentRuntime() {
        const runtime = bridge.currentScriptRuntime();
        return freezeTree(runtime);
    }

    const script = {};
    for (const property of ["metadata", "isBuiltin", "scriptPath", "scriptRoot", "kind", "action", "actionIndex", "method", "methodIndex"]) {
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
        environment,
        MethodLogicError,
        script,
        abort,
    });
})
