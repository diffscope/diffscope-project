// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(bridge => {
    "use strict";

    const storageBridges = new WeakMap();
    const bridgeStorages = new WeakMap();

    class StorageError extends Error {
        constructor(error) {
            super(error.message);
            this.name = "StorageError";
            this.code = error.code;
            this.operation = error.operation;
            this.storageId = error.storageId;
            this.key = error.key;
        }
    }

    const unwrapResult = result => {
        if (!result || result.ok !== true) {
            const error = result && result.error;
            throw new StorageError(error || {
                message: "The host returned an invalid storage result.",
                code: "unknown",
                operation: "unknown",
                storageId: "",
                key: null,
            });
        }
        return result.value;
    };

    const storageBridge = value => {
        const rawStorage = storageBridges.get(value);
        if (!rawStorage) {
            throw new StorageError({
                message: "The storage is not valid in this JavaScript engine.",
                code: "invalidState",
                operation: "access",
                storageId: "",
                key: null,
            });
        }
        return rawStorage;
    };

    class Storage {
        get id() {
            return unwrapResult(storageBridge(this).id());
        }

        get length() {
            return unwrapResult(storageBridge(this).length());
        }

        key(index) {
            if (!Number.isSafeInteger(index) || index < 0) {
                throw new TypeError("index must be a non-negative safe integer.");
            }
            return unwrapResult(storageBridge(this).key(index));
        }

        getItem(key) {
            if (arguments.length < 1) {
                throw new TypeError("getItem requires a key.");
            }
            return unwrapResult(storageBridge(this).getItem(String(key)));
        }

        setItem(key, value) {
            if (arguments.length < 2) {
                throw new TypeError("setItem requires a key and a value.");
            }
            unwrapResult(storageBridge(this).setItem(String(key), String(value)));
        }

        removeItem(key) {
            if (arguments.length < 1) {
                throw new TypeError("removeItem requires a key.");
            }
            unwrapResult(storageBridge(this).removeItem(String(key)));
        }

        clear() {
            unwrapResult(storageBridge(this).clear());
        }
    }

    Object.freeze(Storage.prototype);

    const wrapStorage = rawStorage => {
        if (!rawStorage || typeof rawStorage !== "object") {
            return undefined;
        }
        const existing = bridgeStorages.get(rawStorage);
        if (existing) {
            return existing;
        }
        const value = Object.create(Storage.prototype);
        storageBridges.set(value, rawStorage);
        bridgeStorages.set(rawStorage, value);
        Object.freeze(value);
        return value;
    };

    const unwrapStorage = value => storageBridges.get(value);

    const requireStorageId = id => {
        if (typeof id !== "string" || id.length === 0) {
            throw new TypeError("id must be a non-empty string.");
        }
        return id;
    };

    const storage = Object.freeze({
        session(id) {
            return wrapStorage(unwrapResult(bridge.session(requireStorageId(id))));
        },

        persistent(id) {
            return wrapStorage(unwrapResult(bridge.persistent(requireStorageId(id))));
        },
    });

    const exports = Object.freeze({ StorageError, storage });
    return Object.freeze({ exports, bridge, wrapStorage, unwrapStorage });
})
