// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(bridge => {
    "use strict";

    const handles = new WeakMap();
    const accessValues = Object.freeze({ read: 0, write: 1, readWrite: 2 });
    const dispositionValues = Object.freeze({ replace: 0, createNew: 1, append: 2 });

    class PermissionDeniedError extends Error {
        constructor(message, path) {
            super(message);
            this.name = "PermissionDeniedError";
            this.path = path;
        }
    }

    class FileSystemError extends Error {
        constructor(error) {
            super(error.message);
            this.name = "FileSystemError";
            this.operation = error.operation;
            this.path = error.path === null ? "" : error.path;
            this.destinationPath = error.destinationPath;
            this.code = error.code;
        }
    }

    const unwrapResult = result => {
        if (!result || result.ok !== true) {
            const error = result && result.error;
            if (error && error.permissionDenied) {
                throw new PermissionDeniedError(error.message, error.path);
            }
            throw new FileSystemError(error || {
                message: "The host returned an invalid file-system result.",
                operation: "unknown",
                path: null,
                destinationPath: null,
                code: "unknown",
            });
        }
        return result.value;
    };

    const requireString = (value, name) => {
        if (typeof value !== "string") {
            throw new TypeError(`${name} must be a string.`);
        }
        return value;
    };

    const requireOptions = (value, name) => {
        if (value === undefined) {
            return {};
        }
        if (value === null || typeof value !== "object" || Array.isArray(value)) {
            throw new TypeError(`${name} must be an object.`);
        }
        return value;
    };

    const accessValue = (value, defaultValue = "read") => {
        const access = value === undefined ? defaultValue : value;
        if (!Object.prototype.hasOwnProperty.call(accessValues, access)) {
            throw new TypeError("access must be 'read', 'write', or 'readWrite'.");
        }
        return accessValues[access];
    };

    const dispositionValue = value => {
        const disposition = value === undefined ? "replace" : value;
        if (!Object.prototype.hasOwnProperty.call(dispositionValues, disposition)) {
            throw new TypeError("disposition must be 'replace', 'createNew', or 'append'.");
        }
        return dispositionValues[disposition];
    };

    const optionalBoolean = (value, defaultValue, name) => {
        if (value === undefined) {
            return defaultValue;
        }
        if (typeof value !== "boolean") {
            throw new TypeError(`${name} must be a boolean.`);
        }
        return value;
    };

    const maximumBytes = value => {
        if (value === undefined) {
            return -1;
        }
        if (!Number.isSafeInteger(value) || value < 0) {
            throw new RangeError("maxBytes must be a non-negative safe integer.");
        }
        return value;
    };

    const textReadOptions = value => {
        const options = requireOptions(value, "options");
        return {
            encoding: options.encoding === undefined ? "utf-8" : requireString(options.encoding, "encoding"),
            maxBytes: maximumBytes(options.maxBytes),
        };
    };

    const binaryReadOptions = value => {
        const options = requireOptions(value, "options");
        return { maxBytes: maximumBytes(options.maxBytes) };
    };

    const writeOptions = (value, text) => {
        const options = requireOptions(value, "options");
        const result = {
            disposition: dispositionValue(options.disposition),
            atomic: options.atomic === undefined ? -1 : optionalBoolean(options.atomic, false, "atomic") ? 1 : 0,
            createParents: optionalBoolean(options.createParents, false, "createParents"),
        };
        if (text) {
            result.encoding = options.encoding === undefined ? "utf-8" : requireString(options.encoding, "encoding");
        }
        return result;
    };

    const freezeStat = value => Object.freeze(value);
    const freezeEntries = values => Object.freeze(Array.from(values, value => Object.freeze(value)));

    const wrapHandle = rawHandle => {
        if (!rawHandle || (rawHandle.kind !== "file" && rawHandle.kind !== "directory")) {
            throw new TypeError("The host returned an invalid file-system handle.");
        }

        let handle;
        if (rawHandle.kind === "file") {
            handle = {
                kind: "file",
                path: rawHandle.path,
                access: rawHandle.access,
                stat(options) {
                    const value = requireOptions(options, "options");
                    return freezeStat(unwrapResult(rawHandle.stat(optionalBoolean(value.followSymbolicLinks, true, "followSymbolicLinks"))));
                },
                readText(options) {
                    const value = textReadOptions(options);
                    return unwrapResult(rawHandle.readText(value.encoding, value.maxBytes));
                },
                readBytes(options) {
                    const value = binaryReadOptions(options);
                    return new Uint8Array(unwrapResult(rawHandle.readBytes(value.maxBytes)));
                },
                writeText(text, options) {
                    const value = writeOptions(options, true);
                    unwrapResult(rawHandle.writeText(String(text), value.encoding, value.disposition, value.atomic, value.createParents));
                },
                writeBytes(data, options) {
                    if (!(data instanceof Uint8Array)) {
                        throw new TypeError("data must be a Uint8Array.");
                    }
                    const value = writeOptions(options, false);
                    const snapshot = new Uint8Array(data.byteLength);
                    snapshot.set(data);
                    unwrapResult(rawHandle.writeBytes(snapshot.buffer, value.disposition, value.atomic, value.createParents));
                },
            };
        } else {
            handle = {
                kind: "directory",
                path: rawHandle.path,
                access: rawHandle.access,
                recursive: rawHandle.recursive,
                stat(options) {
                    const value = requireOptions(options, "options");
                    return freezeStat(unwrapResult(rawHandle.stat(optionalBoolean(value.followSymbolicLinks, true, "followSymbolicLinks"))));
                },
                entries(options) {
                    const value = requireOptions(options, "options");
                    let nameFilters = [];
                    if (value.nameFilters !== undefined) {
                        if (!Array.isArray(value.nameFilters)) {
                            throw new TypeError("nameFilters must be an array of strings.");
                        }
                        nameFilters = value.nameFilters.map((filter, index) => requireString(filter, `nameFilters[${index}]`));
                    }
                    return freezeEntries(unwrapResult(rawHandle.entries(
                        optionalBoolean(value.recursive, false, "recursive"),
                        optionalBoolean(value.files, true, "files"),
                        optionalBoolean(value.directories, true, "directories"),
                        nameFilters
                    )));
                },
                file(relativePath, access) {
                    return wrapHandle(unwrapResult(rawHandle.file(requireString(relativePath, "relativePath"), accessValue(access))));
                },
                directory(relativePath) {
                    return wrapHandle(unwrapResult(rawHandle.directory(requireString(relativePath, "relativePath"))));
                },
            };
        }
        handles.set(handle, rawHandle);
        return Object.freeze(handle);
    };

    const unwrapHandle = handle => handles.get(handle);

    const exports = Object.freeze({
        PermissionDeniedError,
        FileSystemError,

        requestPermission(request) {
            const value = requireOptions(request, "request");
            const reason = requireString(value.reason, "reason");
            if (reason.trim().length === 0) {
                throw new TypeError("reason must not be empty.");
            }
            if (value.access === undefined) {
                throw new TypeError("access is required.");
            }
            return unwrapResult(bridge.requestPermission(
                requireString(value.pathPattern, "pathPattern"),
                accessValue(value.access),
                reason
            ));
        },

        stat(path, options) {
            const value = requireOptions(options, "options");
            return freezeStat(unwrapResult(bridge.stat(
                requireString(path, "path"),
                optionalBoolean(value.followSymbolicLinks, true, "followSymbolicLinks")
            )));
        },

        getFile(path, access) {
            return wrapHandle(unwrapResult(bridge.getFile(requireString(path, "path"), accessValue(access))));
        },

        getDirectory(path, options) {
            const value = requireOptions(options, "options");
            return wrapHandle(unwrapResult(bridge.getDirectory(
                requireString(path, "path"),
                accessValue(value.access),
                optionalBoolean(value.recursive, false, "recursive")
            )));
        },

        scriptPackage() {
            return wrapHandle(unwrapResult(bridge.scriptPackage()));
        },

        dataDirectory() {
            return wrapHandle(unwrapResult(bridge.dataDirectory()));
        },

        temporaryDirectory() {
            return wrapHandle(unwrapResult(bridge.temporaryDirectory()));
        },

        createDirectory(path, options) {
            const value = requireOptions(options, "options");
            unwrapResult(bridge.createDirectory(requireString(path, "path"), optionalBoolean(value.recursive, false, "recursive")));
        },

        copy(source, destination, options) {
            const value = requireOptions(options, "options");
            unwrapResult(bridge.copy(
                requireString(source, "source"),
                requireString(destination, "destination"),
                optionalBoolean(value.overwrite, false, "overwrite"),
                optionalBoolean(value.recursive, false, "recursive")
            ));
        },

        move(source, destination, options) {
            const value = requireOptions(options, "options");
            unwrapResult(bridge.move(
                requireString(source, "source"),
                requireString(destination, "destination"),
                optionalBoolean(value.overwrite, false, "overwrite")
            ));
        },

        remove(path, options) {
            const value = requireOptions(options, "options");
            unwrapResult(bridge.remove(
                requireString(path, "path"),
                optionalBoolean(value.recursive, false, "recursive"),
                optionalBoolean(value.useTrash, true, "useTrash")
            ));
        },

        realPath(path) {
            return unwrapResult(bridge.realPath(requireString(path, "path")));
        },

        normalize(path) {
            return bridge.normalize(requireString(path, "path"));
        },

        resolve(...parts) {
            return unwrapResult(bridge.resolve(parts.map((part, index) => requireString(part, `parts[${index}]`))));
        },

        relative(from, to) {
            return unwrapResult(bridge.relative(requireString(from, "from"), requireString(to, "to")));
        },

        isAbsolute(path) {
            return bridge.isAbsolute(requireString(path, "path"));
        },

        join(...parts) {
            return bridge.join(parts.map((part, index) => requireString(part, `parts[${index}]`)));
        },

        basename(path) {
            return bridge.baseName(requireString(path, "path"));
        },

        dirname(path) {
            return bridge.directoryName(requireString(path, "path"));
        },

        extension(path) {
            return bridge.extension(requireString(path, "path"));
        },

        parse(path) {
            return Object.freeze(bridge.parse(requireString(path, "path")));
        },

        format(path) {
            const value = requireOptions(path, "path");
            return unwrapResult(bridge.format(
                requireString(value.root, "root"),
                requireString(value.directory, "directory"),
                requireString(value.baseName, "baseName"),
                requireString(value.name, "name"),
                requireString(value.extension, "extension")
            ));
        },

        matchesPattern(path, pattern) {
            return unwrapResult(bridge.matchesPattern(requireString(path, "path"), requireString(pattern, "pattern")));
        },
    });

    const bundle = Object.freeze({ exports, bridge, wrapHandle, unwrapHandle });
    return bundle;
})
