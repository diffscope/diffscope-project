// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(bridge => {
    "use strict";

    const arrayIsArray = Array.isArray;
    const freeze = Object.freeze;
    const string = String;

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

    const translationInstances = new Map();

    function normalizedLanguageCode(value) {
        return string(value).trim().replace(/_/g, "-").toLowerCase();
    }

    function currentScriptKey() {
        const key = bridge.currentScriptIdentity();
        if (!key) {
            throw new Error("The current script is no longer active.");
        }
        return key;
    }

    function loadI18n(directory, fallbackLanguage) {
        const fallback = fallbackLanguage === undefined || fallbackLanguage === null ? "en" : string(fallbackLanguage);
        const directoryInfo = bridge.prepareI18n(string(directory), fallback);
        const scriptKey = currentScriptKey();
        if (!directoryInfo.language) {
            translationInstances.set(scriptKey, null);
            return;
        }

        const instance = bridge.createI18nextInstance();
        const selectedLanguage = normalizedLanguageCode(directoryInfo.language);
        instance.use({
            type: "backend",
            callbackApi: false,
            init() {
            },
            read(language, namespace) {
                if (namespace !== "translation" || normalizedLanguageCode(language) !== selectedLanguage) {
                    return undefined;
                }
                return bridge.readI18nResource(directoryInfo.directory, language);
            },
        });

        let initError;
        instance.init({
            lng: directoryInfo.language,
            fallbackLng: false,
            load: "currentOnly",
            initAsync: false,
            ns: ["translation"],
            defaultNS: "translation",
        }, error => {
            initError = error;
        });
        if (initError) {
            translationInstances.delete(scriptKey);
            const error = arrayIsArray(initError) ? initError[0] : initError;
            throw error instanceof Error ? error : new Error(string(error));
        }
        translationInstances.set(scriptKey, instance);
    }

    function translate(scriptKey, ...args) {
        const instance = translationInstances.get(scriptKey);
        if (instance) {
            return instance.t(...args);
        }
        const key = args[0];
        return typeof key === "string" ? key : "";
    }

    function t(...args) {
        return translate(currentScriptKey(), ...args);
    }

    return {
        exports: freeze({
            environment,
            MethodLogicError,
            script,
            abort,
            loadI18n,
            t,
        }),
        translate,
    };
})
