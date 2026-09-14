// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(bridge => {
    "use strict";

    const thisFileSourceUrl = "qrc:/diffscope/batchprocess/internalscripts/runtime/console.js";
    const timers = new Map();

    const level = Object.freeze({
        debug: 0,
        log: 1,
        info: 2,
        warning: 3,
        error: 4,
    });

    function convertInteger(value) {
        if (typeof value === "symbol") {
            return NaN;
        }
        return typeof value === "bigint" ? value : parseInt(value, 10);
    }

    function convertFloat(value) {
        return typeof value === "symbol" ? NaN : parseFloat(value);
    }

    function format(data) {
        if (data.length === 0) {
            return "";
        }

        const args = Array.from(data);
        if (typeof args[0] !== "string" || args.length === 1) {
            return args.map(value => String(value)).join(" ");
        }

        let currentArgument = 1;
        const formatted = args[0].replace(/%[sdifoOc%]/g, specifier => {
            if (specifier === "%%") {
                return "%";
            }
            if (currentArgument >= args.length) {
                return specifier;
            }

            const value = args[currentArgument++];
            switch (specifier) {
                case "%d":
                case "%i":
                    return String(convertInteger(value));
                case "%f":
                    return String(convertFloat(value));
                case "%c":
                    return "";
                case "%s":
                case "%o":
                case "%O":
                    return String(value);
                default:
                    return specifier;
            }
        });

        return [formatted, ...args.slice(currentArgument)].map(value => String(value)).join(" ");
    }

    function scriptStack() {
        return bridge.stackTrace().filter(frame => {
            return frame.fileUrl !== thisFileSourceUrl;
        });
    }

    function append(outputLevel, text, stack) {
        const frame = stack.length === 0 ? undefined : stack[0];
        bridge.appendMessage(
            outputLevel,
            text,
            frame === undefined ? "" : frame.fileUrl,
            frame === undefined ? -1 : frame.line,
            frame === undefined ? -1 : frame.column
        );
    }

    function write(outputLevel, data) {
        if (data.length === 0) {
            return;
        }
        append(outputLevel, format(data), scriptStack());
    }

    function frameText(frame) {
        const functionName = frame.functionName || "<anonymous>";
        let source = frame.displaySource;
        if (source && frame.line > 0) {
            source += `:${frame.line}`;
            if (frame.column > 0) {
                source += `:${frame.column}`;
            }
        }
        return source ? `    at ${functionName} (${source})` : `    at ${functionName}`;
    }

    function timerLabel(value) {
        return value === undefined ? "default" : String(value);
    }

    function durationText(milliseconds) {
        return String(Math.round(milliseconds * 1000) / 1000);
    }

    const console = {
        debug(...data) {
            write(level.debug, data);
        },
        log(...data) {
            write(level.log, data);
        },
        info(...data) {
            write(level.info, data);
        },
        warn(...data) {
            write(level.warning, data);
        },
        error(...data) {
            write(level.error, data);
        },
        assert(condition = false, ...data) {
            if (condition) {
                return;
            }
            if (data.length === 0) {
                data.push("Assertion failed");
            } else if (typeof data[0] === "string") {
                data[0] = `Assertion failed: ${data[0]}`;
            } else {
                data.unshift("Assertion failed");
            }
            write(level.error, data);
        },
        time(label = "default") {
            label = timerLabel(label);
            if (timers.has(label)) {
                write(level.warning, [`Timer '${label}' already exists`]);
                return;
            }
            timers.set(label, bridge.monotonicMilliseconds());
        },
        timeEnd(label = "default") {
            label = timerLabel(label);
            if (!timers.has(label)) {
                write(level.warning, [`Timer '${label}' does not exist`]);
                return;
            }
            const start = timers.get(label);
            timers.delete(label);
            write(level.info, [`${label}: ${durationText(bridge.monotonicMilliseconds() - start)} ms`]);
        },
        trace(...data) {
            const stack = scriptStack();
            const trace = stack.map(frameText).join("\n");
            const label = data.length === 0 ? "Trace" : format(data);
            append(level.log, trace ? `${label}\n${trace}` : label, stack);
        },
    };

    return Object.freeze(console);
})
