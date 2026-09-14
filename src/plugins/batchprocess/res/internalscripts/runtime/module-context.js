// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(() => {
    "use strict";

    const createObject = Object.create;
    const defineProperties = Object.defineProperties;
    const defineProperty = Object.defineProperty;
    const freeze = Object.freeze;

    function freezeDefinition(definition) {
        const actions = definition.actions;
        for (let index = 0; index < actions.length; ++index) {
            freeze(actions[index]);
        }
        freeze(actions);
        return freeze(definition);
    }

    function makeDefineScript(bridge) {
        return definition => freezeDefinition(bridge.defineScript(definition));
    }

    function makeRequire(bridge) {
        return specifier => bridge.load(specifier);
    }

    function makeModule(id, exports) {
        const module = createObject(null);
        defineProperties(module, {
            id: {
                value: id,
                writable: false,
                configurable: false,
                enumerable: true,
            },
            exports: {
                value: exports,
                writable: true,
                configurable: false,
                enumerable: true,
            },
        });
        return module;
    }

    function defineReadOnlyGlobal(globalObject, name, value) {
        defineProperty(globalObject, name, {
            value,
            writable: false,
            configurable: false,
            enumerable: true,
        });
    }

    function invokeModule(factory, module, require, defineScript) {
        try {
            factory(module, require, defineScript);
            return { ok: true };
        } catch (error) {
            return { ok: false, error };
        }
    }

    function invokeAction(handler) {
        try {
            return { ok: true, value: handler() };
        } catch (error) {
            return { ok: false, error };
        }
    }

    return freeze({
        defineReadOnlyGlobal,
        invokeAction,
        invokeModule,
        makeDefineScript,
        makeModule,
        makeRequire,
    });
})()
