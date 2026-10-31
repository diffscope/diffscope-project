// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(() => {
    "use strict";

    const createObject = Object.create;
    const defineProperties = Object.defineProperties;
    const defineProperty = Object.defineProperty;
    const freeze = Object.freeze;
    const arrayIsArray = Array.isArray;
    const stringify = JSON.stringify;
    const string = String;
    const Type = TypeError;
    const methodNamePattern = /^[$_\p{ID_Start}][$_\u200C\u200D\p{ID_Continue}]*(?:\.[$_\p{ID_Start}][$_\u200C\u200D\p{ID_Continue}]*)*(?![\s\S])/u;

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

    function freezeDefinition(definition) {
        const actions = definition.actions;
        for (let index = 0; index < actions.length; ++index) {
            freeze(actions[index]);
        }
        freeze(actions);
        if (definition.methods !== undefined) {
            for (const method of definition.methods) {
                freezeTree(method.params);
                freezeTree(method.result);
                freezeTree(method.annotations);
                freeze(method);
            }
            freeze(definition.methods);
        }
        return freeze(definition);
    }

    function encodeJson(value) {
        if (value === undefined) {
            return undefined;
        }
        const json = stringify(value);
        if (json === undefined) {
            throw new Type("The RPC value cannot be represented as JSON.");
        }
        return json;
    }

    function buildMethodDefinitions(methods) {
        if (methods === undefined) {
            return [];
        }
        if (!arrayIsArray(methods)) {
            throw new Type("Script property methods must be an array.");
        }
        const names = new Set();
        const definitions = [];
        const optionalText = (source, target, key) => {
            const value = source[key];
            if (value !== undefined) {
                if (typeof value !== "string") {
                    throw new Type("Method metadata property " + key + " must be a string.");
                }
                target[key] = value;
            }
        };
        const booleanValue = (source, key, fallback) => {
            const value = source[key];
            if (value !== undefined && typeof value !== "boolean") {
                throw new Type("Method metadata property " + key + " must be a boolean.");
            }
            return value === undefined ? fallback : value;
        };
        const descriptor = (value, parameter) => {
            if (value === null || typeof value !== "object" || arrayIsArray(value) || typeof value.name !== "string") {
                throw new Type("Method descriptors must be objects with a string name.");
            }
            const schema = value.schema;
            if (typeof schema !== "boolean" && (schema === null || typeof schema !== "object" || arrayIsArray(schema))) {
                throw new Type("Method descriptor schema must be a boolean or an object.");
            }
            const result = { name: value.name, schema };
            optionalText(value, result, "summary");
            optionalText(value, result, "description");
            if (parameter) {
                result.required = booleanValue(value, "required", false);
            }
            return result;
        };
        for (const method of methods) {
            if (method === null || typeof method !== "object" || arrayIsArray(method)) {
                throw new Type("Script methods must be objects.");
            }
            const name = method.name;
            if (typeof name !== "string" || !methodNamePattern.test(name)) {
                throw new Type("Method names must be JavaScript identifier names separated by dots.");
            }
            if (names.has(name)) {
                throw new Type("Method names must be unique within a script.");
            }
            names.add(name);
            if (!arrayIsArray(method.params)) {
                throw new Type("Method property params must be an array.");
            }
            const execute = method.execute;
            if (typeof execute !== "function") {
                throw new Type("Method property execute must be a function.");
            }
            const metadata = { name, params: [] };
            optionalText(method, metadata, "summary");
            optionalText(method, metadata, "description");
            const parameterNames = new Set();
            for (const param of method.params) {
                const parameter = descriptor(param, true);
                if (parameterNames.has(parameter.name)) {
                    throw new Type("Parameter names must be unique within a method.");
                }
                parameterNames.add(parameter.name);
                metadata.params.push(parameter);
            }
            if (method.result !== undefined) {
                metadata.result = descriptor(method.result, false);
            }
            const annotations = method.annotations === undefined ? {} : method.annotations;
            if (annotations === null || typeof annotations !== "object" || arrayIsArray(annotations)) {
                throw new Type("Method property annotations must be an object.");
            }
            metadata.annotations = {
                readOnlyHint: booleanValue(annotations, "readOnlyHint", false),
                destructiveHint: booleanValue(annotations, "destructiveHint", true),
                idempotentHint: booleanValue(annotations, "idempotentHint", false),
                openWorldHint: booleanValue(annotations, "openWorldHint", true),
            };
            definitions.push({ metadata: encodeJson(metadata), execute });
        }
        return definitions;
    }

    function methodDefinitions(definition) {
        try {
            return { ok: true, value: buildMethodDefinitions(definition.methods) };
        } catch (error) {
            return { ok: false, error };
        }
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

    function methodError(error, errorType) {
        let logicError = false;
        let name = "";
        let message = "The thrown value has no readable message.";
        let data;
        try {
            logicError = errorType !== undefined && error instanceof errorType;
        } catch (_) {
        }
        try {
            if (error !== null && error !== undefined && typeof error.name === "string") {
                name = error.name;
            }
        } catch (_) {
        }
        try {
            message = error !== null && error !== undefined && typeof error.message === "string" ? error.message : string(error);
        } catch (_) {
        }
        if (logicError) {
            try {
                data = encodeJson(error.data);
            } catch (_) {
            }
        }
        return { ok: false, logicError, name, message, data };
    }

    function invokeMethod(handler, params, errorType) {
        let value;
        try {
            value = handler(params);
        } catch (error) {
            return methodError(error, errorType);
        }
        try {
            if (value !== null && (typeof value === "object" || typeof value === "function") && typeof value.then === "function") {
                throw new Type("Batch Process methods must complete synchronously.");
            }
            return { ok: true, value: encodeJson(value) };
        } catch (error) {
            return methodError(error, errorType);
        }
    }

    return freeze({
        defineReadOnlyGlobal,
        invokeAction,
        invokeMethod,
        invokeModule,
        makeDefineScript,
        makeModule,
        makeRequire,
        methodDefinitions,
    });
})()
