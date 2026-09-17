// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(bridge => {
    "use strict";

    const windowBridges = new WeakMap();
    const bridgeWindows = new WeakMap();
    const severities = Object.freeze(["information", "success", "warning", "critical", "question"]);
    const notificationSeverities = Object.freeze(["information", "success", "warning", "critical"]);
    const buttonRoles = Object.freeze(["accept", "reject", "destructive", "help", "action"]);
    const accessModes = Object.freeze(["read", "write", "readWrite"]);
    const bubbleModes = Object.freeze(["normal", "hidden", "autoHide"]);

    class WindowError extends Error {
        constructor(message, code, window) {
            super(message);
            this.name = "WindowError";
            this.code = code;
            this.window = window;
        }
    }

    const unwrapResult = (result, window = null) => {
        if (!result || result.ok !== true) {
            const error = result && result.error;
            if (error && error.windowError === true) {
                throw new WindowError(error.message, error.code, window);
            }
            throw new Error(error && typeof error.message === "string"
                ? error.message
                : "The shell host returned an invalid result.");
        }
        return result.value;
    };

    class Window {
        get valid() {
            const handle = windowBridges.get(this);
            return handle ? handle.valid === true : false;
        }

        get active() {
            const handle = windowBridges.get(this);
            return handle ? handle.active === true : false;
        }

        raise() {
            const handle = windowBridges.get(this);
            if (!handle) {
                throw new WindowError("The application window handle is invalid.", "invalidWindow", this);
            }
            unwrapResult(handle.raise(), this);
        }
    }

    const wrapWindow = handle => {
        if (!handle || typeof handle !== "object") {
            return undefined;
        }
        const existing = bridgeWindows.get(handle);
        if (existing) {
            return existing;
        }
        const value = Object.create(Window.prototype);
        windowBridges.set(value, handle);
        bridgeWindows.set(handle, value);
        Object.freeze(value);
        return value;
    };

    const unwrapWindow = value => windowBridges.get(value);

    const requireObject = (value, name) => {
        if (value === null || typeof value !== "object" || Array.isArray(value)) {
            throw new TypeError(`${name} must be an object.`);
        }
        return value;
    };

    const optionsObject = (value, name = "options") => value === undefined ? {} : requireObject(value, name);

    const requireString = (value, name, nonEmpty = false) => {
        if (typeof value !== "string" || (nonEmpty && value.length === 0)) {
            throw new TypeError(`${name} must be ${nonEmpty ? "a non-empty string" : "a string"}.`);
        }
        return value;
    };

    const optionalString = (value, name, defaultValue = "") => value === undefined
        ? defaultValue
        : requireString(value, name);

    const optionalBoolean = (value, name, defaultValue = false) => {
        if (value === undefined) {
            return defaultValue;
        }
        if (typeof value !== "boolean") {
            throw new TypeError(`${name} must be a boolean.`);
        }
        return value;
    };

    const enumValue = (value, allowed, name, defaultValue) => {
        const result = value === undefined ? defaultValue : value;
        if (typeof result !== "string" || !allowed.includes(result)) {
            throw new TypeError(`${name} has an invalid value.`);
        }
        return result;
    };

    const parentBridge = value => {
        if (value === undefined || value === null) {
            return null;
        }
        const handle = windowBridges.get(value);
        if (!handle) {
            throw new TypeError("parent must be a Window, null, or undefined.");
        }
        return handle;
    };

    const normalizeButtons = buttons => {
        if (buttons === undefined) {
            return undefined;
        }
        if (!Array.isArray(buttons) || buttons.length === 0) {
            throw new TypeError("buttons must be a non-empty array when provided.");
        }
        const ids = new Set();
        let defaultCount = 0;
        let escapeCount = 0;
        return buttons.map((button, index) => {
            const value = requireObject(button, `buttons[${index}]`);
            const id = requireString(value.id, `buttons[${index}].id`, true);
            if (ids.has(id)) {
                throw new TypeError("button IDs must be unique.");
            }
            ids.add(id);
            const isDefault = optionalBoolean(value.default, `buttons[${index}].default`);
            const isEscape = optionalBoolean(value.escape, `buttons[${index}].escape`);
            defaultCount += isDefault ? 1 : 0;
            escapeCount += isEscape ? 1 : 0;
            if (defaultCount > 1 || escapeCount > 1) {
                throw new TypeError("buttons may contain at most one default and one escape button.");
            }
            return Object.freeze({
                id,
                text: requireString(value.text, `buttons[${index}].text`, true),
                role: enumValue(value.role, buttonRoles, `buttons[${index}].role`, "action"),
                default: isDefault,
                escape: isEscape,
            });
        });
    };

    const normalizeChoiceItems = (items, name) => {
        if (!Array.isArray(items)) {
            throw new TypeError(`${name} must be an array.`);
        }
        const values = new Set();
        return items.map((item, index) => {
            const value = requireObject(item, `${name}[${index}]`);
            const stableValue = requireString(value.value, `${name}[${index}].value`);
            if (values.has(stableValue)) {
                throw new TypeError(`${name} values must be unique.`);
            }
            values.add(stableValue);
            return Object.freeze({
                value: stableValue,
                label: requireString(value.label, `${name}[${index}].label`, true),
                description: optionalString(value.description, `${name}[${index}].description`),
                enabled: optionalBoolean(value.enabled, `${name}[${index}].enabled`, true),
            });
        });
    };

    const finiteNumber = (value, name) => {
        if (typeof value !== "number" || !Number.isFinite(value)) {
            throw new TypeError(`${name} must be a finite number.`);
        }
        return value;
    };

    const safeInteger = (value, name) => {
        if (!Number.isSafeInteger(value)) {
            throw new TypeError(`${name} must be a safe integer.`);
        }
        return value;
    };

    const normalizeColor = (value, name) => {
        if (value === undefined) {
            return Object.freeze({ red: 0, green: 0, blue: 0, alpha: 1 });
        }
        const color = requireObject(value, name);
        const result = {};
        for (const channel of ["red", "green", "blue", "alpha"]) {
            const channelValue = finiteNumber(color[channel], `${name}.${channel}`);
            if (channelValue < 0 || channelValue > 1) {
                throw new TypeError(`${name}.${channel} must be between 0 and 1.`);
            }
            result[channel] = channelValue;
        }
        return Object.freeze(result);
    };

    const createFormField = (base, type, properties) => {
        const result = {
            name: base.name,
            label: base.label,
            description: base.description,
            required: base.required,
            type,
        };
        for (const name of Object.keys(properties)) {
            result[name] = properties[name];
        }
        return Object.freeze(result);
    };

    const normalizeFormField = (field, index) => {
        const value = requireObject(field, `fields[${index}]`);
        const base = {
            name: requireString(value.name, `fields[${index}].name`, true),
            label: requireString(value.label, `fields[${index}].label`, true),
            description: optionalString(value.description, `fields[${index}].description`),
            required: optionalBoolean(value.required, `fields[${index}].required`),
        };
        const type = requireString(value.type, `fields[${index}].type`, true);
        if (type === "text") {
            const minimumLength = value.minLength === undefined ? 0 : safeInteger(value.minLength, `fields[${index}].minLength`);
            const hasMaximumLength = value.maxLength !== undefined;
            const maximumLength = hasMaximumLength ? safeInteger(value.maxLength, `fields[${index}].maxLength`) : -1;
            if (minimumLength < 0 || (hasMaximumLength && maximumLength < minimumLength)) {
                throw new TypeError(`fields[${index}] has invalid text length limits.`);
            }
            const pattern = optionalString(value.pattern, `fields[${index}].pattern`);
            if (pattern.length !== 0) {
                try {
                    new RegExp(`^(?:${pattern})$`);
                } catch (error) {
                    throw new TypeError(`fields[${index}].pattern is not a valid ECMAScript regular expression.`);
                }
            }
            return createFormField(base, type, {
                value: optionalString(value.value, `fields[${index}].value`),
                placeholder: optionalString(value.placeholder, `fields[${index}].placeholder`),
                multiline: optionalBoolean(value.multiline, `fields[${index}].multiline`),
                minLength: minimumLength,
                maxLength: maximumLength,
                pattern,
            });
        }
        if (type === "integer") {
            const minimum = value.minimum === undefined ? 0 : safeInteger(value.minimum, `fields[${index}].minimum`);
            const maximum = value.maximum === undefined ? 100 : safeInteger(value.maximum, `fields[${index}].maximum`);
            const step = value.step === undefined ? 1 : safeInteger(value.step, `fields[${index}].step`);
            if (minimum > maximum || step <= 0) {
                throw new TypeError(`fields[${index}] has an invalid integer range or step.`);
            }
            const initialValue = value.value === undefined
                ? Math.min(maximum, Math.max(minimum, 0))
                : safeInteger(value.value, `fields[${index}].value`);
            if (initialValue < minimum || initialValue > maximum) {
                throw new TypeError(`fields[${index}].value is outside its allowed range.`);
            }
            return createFormField(base, type, { value: initialValue, minimum, maximum, step });
        }
        if (type === "number") {
            const minimum = value.minimum === undefined ? 0 : finiteNumber(value.minimum, `fields[${index}].minimum`);
            const maximum = value.maximum === undefined ? 100 : finiteNumber(value.maximum, `fields[${index}].maximum`);
            const step = value.step === undefined ? 0.01 : finiteNumber(value.step, `fields[${index}].step`);
            const decimals = value.decimals === undefined ? 2 : safeInteger(value.decimals, `fields[${index}].decimals`);
            if (minimum > maximum || step <= 0 || decimals < 0 || decimals > 15) {
                throw new TypeError(`fields[${index}] has an invalid number range, step, or precision.`);
            }
            const initialValue = value.value === undefined
                ? Math.min(maximum, Math.max(minimum, 0))
                : finiteNumber(value.value, `fields[${index}].value`);
            if (initialValue < minimum || initialValue > maximum) {
                throw new TypeError(`fields[${index}].value is outside its allowed range.`);
            }
            return createFormField(base, type, { value: initialValue, minimum, maximum, step, decimals });
        }
        if (type === "boolean") {
            return createFormField(base, type, {
                value: optionalBoolean(value.value, `fields[${index}].value`),
            });
        }
        if (type === "choice") {
            const items = normalizeChoiceItems(value.items, `fields[${index}].items`);
            const firstEnabled = items.find(item => item.enabled);
            const requestedValue = value.value === undefined ? undefined : requireString(value.value, `fields[${index}].value`);
            const selected = requestedValue === undefined ? firstEnabled : items.find(item => item.value === requestedValue && item.enabled);
            if (base.required && !firstEnabled) {
                throw new TypeError(`fields[${index}] requires at least one enabled choice.`);
            }
            return createFormField(base, type, {
                items: Object.freeze(items),
                value: selected ? selected.value : (firstEnabled ? firstEnabled.value : ""),
            });
        }
        if (type === "color") {
            return createFormField(base, type, {
                value: normalizeColor(value.value, `fields[${index}].value`),
                alpha: optionalBoolean(value.alpha, `fields[${index}].alpha`),
            });
        }
        throw new TypeError(`fields[${index}].type has an invalid value.`);
    };

    const normalizeFormFields = fields => {
        if (!Array.isArray(fields)) {
            throw new TypeError("fields must be an array.");
        }
        const names = new Set();
        const result = fields.map((field, index) => {
            const normalized = normalizeFormField(field, index);
            if (names.has(normalized.name)) {
                throw new TypeError("field names must be unique.");
            }
            names.add(normalized.name);
            return normalized;
        });
        return Object.freeze(result);
    };

    const normalizeFilters = filters => {
        if (filters === undefined) {
            return Object.freeze([]);
        }
        if (!Array.isArray(filters)) {
            throw new TypeError("filters must be an array.");
        }
        return Object.freeze(filters.map((filter, index) => {
            const value = requireObject(filter, `filters[${index}]`);
            if (!Array.isArray(value.patterns) || value.patterns.length === 0) {
                throw new TypeError(`filters[${index}].patterns must be a non-empty array.`);
            }
            return Object.freeze({
                name: requireString(value.name, `filters[${index}].name`, true),
                patterns: Object.freeze(value.patterns.map((pattern, patternIndex) => requireString(pattern, `filters[${index}].patterns[${patternIndex}]`, true))),
            });
        }));
    };

    const dialogs = Object.freeze({
        message(options) {
            const value = requireObject(options, "options");
            const parent = value.parent;
            return unwrapResult(bridge.message(
                requireString(value.title, "title"),
                requireString(value.message, "message"),
                optionalString(value.detail, "detail"),
                enumValue(value.severity, severities, "severity", "information"),
                normalizeButtons(value.buttons),
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
        },

        alert(message, options) {
            const value = optionsObject(options);
            const parent = value.parent;
            unwrapResult(bridge.message(
                optionalString(value.title, "title", bridge.scriptName),
                requireString(message, "message"),
                "",
                enumValue(value.severity, notificationSeverities, "severity", "information"),
                undefined,
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
        },

        confirm(message, options) {
            const value = optionsObject(options);
            const parent = value.parent;
            const destructive = optionalBoolean(value.destructive, "destructive");
            const result = unwrapResult(bridge.message(
                optionalString(value.title, "title", bridge.scriptName),
                requireString(message, "message"),
                "",
                destructive ? "warning" : "question",
                [
                    Object.freeze({ id: "accept", text: optionalString(value.acceptText, "acceptText", bridge.okText), role: destructive ? "destructive" : "accept", default: true, escape: false }),
                    Object.freeze({ id: "cancel", text: optionalString(value.cancelText, "cancelText", bridge.cancelText), role: "reject", default: false, escape: true }),
                ],
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
            return result === "accept";
        },

        promptText(options) {
            const value = requireObject(options, "options");
            const parent = value.parent;
            return unwrapResult(bridge.promptText(
                requireString(value.label, "label", true),
                optionalString(value.title, "title"),
                value.title !== undefined,
                optionalString(value.value, "value"),
                optionalString(value.placeholder, "placeholder"),
                optionalBoolean(value.multiline, "multiline"),
                optionalBoolean(value.allowEmpty, "allowEmpty", true),
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
        },

        choose(options) {
            const value = requireObject(options, "options");
            const parent = value.parent;
            return unwrapResult(bridge.choose(
                requireString(value.title, "title", true),
                optionalString(value.placeholder, "placeholder"),
                normalizeChoiceItems(value.items, "items"),
                optionalString(value.initialValue, "initialValue"),
                optionalString(value.initialFilter, "initialFilter"),
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
        },

        form(options) {
            const value = requireObject(options, "options");
            const parent = value.parent;
            const fields = normalizeFormFields(value.fields);
            const values = unwrapResult(bridge.form(
                requireString(value.title, "title", true),
                optionalString(value.description, "description"),
                fields,
                optionalString(value.acceptText, "acceptText", bridge.okText),
                optionalString(value.cancelText, "cancelText", bridge.cancelText),
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
            if (values === null) {
                return null;
            }
            const result = Object.create(null);
            fields.forEach((field, index) => {
                const fieldValue = field.type === "color" ? Object.freeze(values[index]) : values[index];
                Object.defineProperty(result, field.name, {
                    value: fieldValue,
                    enumerable: true,
                    configurable: false,
                    writable: false,
                });
            });
            return Object.freeze(result);
        },

        openFile(options) {
            const value = optionsObject(options);
            const parent = value.parent;
            const multiple = optionalBoolean(value.multiple, "multiple");
            const result = unwrapResult(bridge.openFile(
                optionalString(value.title, "title"),
                optionalString(value.initialPath, "initialPath"),
                normalizeFilters(value.filters),
                multiple,
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
            return multiple && result !== null ? Object.freeze(result) : result;
        },

        saveFile(options) {
            const value = optionsObject(options);
            const parent = value.parent;
            return unwrapResult(bridge.saveFile(
                optionalString(value.title, "title"),
                optionalString(value.initialPath, "initialPath"),
                optionalString(value.suggestedName, "suggestedName"),
                normalizeFilters(value.filters),
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
        },

        selectDirectory(options) {
            const value = optionsObject(options);
            const parent = value.parent;
            return unwrapResult(bridge.selectDirectory(
                optionalString(value.title, "title"),
                optionalString(value.initialPath, "initialPath"),
                optionalBoolean(value.recursive, "recursive"),
                enumValue(value.access, accessModes, "access", "read"),
                parentBridge(parent)
            ), parent instanceof Window ? parent : null);
        },
    });

    const notifications = Object.freeze({
        post(options) {
            const value = requireObject(options, "options");
            const targetWindow = value.window;
            unwrapResult(bridge.postNotification(
                requireString(value.title, "title"),
                requireString(value.message, "message"),
                enumValue(value.severity, notificationSeverities, "severity", "information"),
                enumValue(value.bubble, bubbleModes, "bubble", "normal"),
                parentBridge(targetWindow),
                targetWindow instanceof Window
            ), targetWindow instanceof Window ? targetWindow : null);
        },
    });

    Object.freeze(Window.prototype);
    Object.freeze(WindowError.prototype);
    Object.freeze(WindowError);

    const exports = Object.freeze({ dialogs, notifications, WindowError });
    return Object.freeze({ exports, bridge, wrapWindow, unwrapWindow });
})
