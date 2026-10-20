// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

.pragma library

function normalize(value) {
    if (Array.isArray(value))
        return value.map(normalize)
    if (value === null || typeof value !== "object")
        return value
    const normalized = {}
    for (const key of Object.keys(value).sort()) {
        Object.defineProperty(normalized, key, {
            value: normalize(value[key]),
            enumerable: true,
            writable: true,
            configurable: true
        })
    }
    return normalized
}

function copy(value) {
    const encoded = JSON.stringify(value, function(key, entry) {
        const kind = typeof entry
        if (kind === "undefined" || kind === "function" || kind === "symbol"
                || kind === "bigint" || (kind === "number" && !Number.isFinite(entry)))
            throw new Error("The control panel state contains a non-JSON value.")
        return entry
    })
    return normalize(JSON.parse(encoded))
}
