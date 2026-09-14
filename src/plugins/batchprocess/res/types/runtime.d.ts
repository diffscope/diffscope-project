// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

interface ScriptMetadata {
    /**
     * Must be non-empty. The ID is unique across all built-in and user scripts in
     * an engine. On collision, the host keeps the script loaded first.
     */
    readonly id: string;

    /** Must be non-empty. */
    readonly name: string;

    /**
     * Must be non-empty. SemVer is recommended, but the host does not use this
     * value to select API behavior.
     */
    readonly version: string;

    readonly description: string;

    readonly author?: string;
}

interface ScriptActionMetadata {
    /** Must be non-empty and unique within the containing script. */
    readonly name: string;

    readonly description: string;

    /**
     * Defaults to `false`. If `true`, the host does not invoke the action without
     * an associated project and informs the user instead. Runtime metadata
     * snapshots contain the normalized Boolean value.
     */
    readonly requiresProject?: boolean;
}

/**
 * A handler runs synchronously on the GUI thread. Returning normally completes
 * the action; throwing any JavaScript value fails it. A handler must not return a
 * Promise or another thenable, which the host treats as a contract violation.
 */
type ScriptActionHandler = () => void;

interface ScriptAction extends ScriptActionMetadata {
    readonly execute: ScriptActionHandler;
}

interface ScriptDefinition extends ScriptMetadata {
    /**
     * The host preserves declaration order and freezes this array and each of its
     * elements after validation.
     */
    readonly actions: readonly ScriptAction[];
}

/**
 * Validates and registers a script definition during entry-script evaluation.
 * On success, the host freezes the definition, its actions array, and every
 * action, then returns the original object. The entry script must export that
 * returned object.
 *
 * Invalid definitions cause a `TypeError` and are not registered. Top-level code
 * must not perform user interaction, modify documents, or access files; such work
 * belongs in an action handler.
 *
 * @param definition The complete definition to validate and register.
 * @returns The validated and frozen input object.
 */
declare function defineScript<const T extends ScriptDefinition>(definition: T): Readonly<T>;

declare module "diffscope:runtime" {
    export type Platform = "windows" | "macos" | "linux";

    export type ScriptActionMetadata = Omit<ScriptAction, "execute">;

    export type ScriptMetadata = Omit<ScriptDefinition, "actions"> & {
        /** Preserves declaration order. */
        readonly actions: readonly ScriptActionMetadata[];
    };

    export interface ScriptRuntimeBase {
        /**
         * A frozen snapshot. Its actions array and action metadata objects are
         * frozen as well.
         */
        readonly metadata: Readonly<ScriptMetadata>;

        /**
         * The native absolute entry-file path for a single-file script, the native
         * absolute package-directory path for a multi-file script, or an empty
         * string for a built-in script.
         */
        readonly scriptPath: string;

        /**
         * The configured Scripts directory for a single-file script, the package
         * directory for a multi-file script, or an empty string for a built-in
         * script. This is the package-root boundary used for file resolution.
         */
        readonly scriptRoot: string;

        /** The current application locale as a BCP 47 language tag. */
        readonly locale: string;

        readonly platform: Platform;
    }

    export interface ActionScriptRuntime extends ScriptRuntimeBase {
        readonly kind: "action";

        /** A frozen snapshot. */
        readonly action: Readonly<ScriptActionMetadata>;

        /** The zero-based index of `action` in `metadata.actions`. */
        readonly actionIndex: number;
    }

    export type ScriptRuntime = ActionScriptRuntime;

    /**
     * A fixed, frozen facade whose identity remains stable for the lifetime of an
     * engine. Each property is a getter that resolves the current execution
     * context when read.
     *
     * The facade itself may be captured during entry-script evaluation, but its
     * properties may only be read in the synchronous call chain of an executing
     * action. Reading a property at any other time throws an exception.
     */
    export const script: ScriptRuntime;

    /**
     * Immediately interrupts the currently executing action.
     *
     * An action must be executing when this function is called; otherwise it
     * throws. A successful call never returns to JavaScript, and statements after
     * it in the action are not executed. The host reports the action as failed and
     * clears the interruption state when execution finishes, so later actions are
     * unaffected.
     *
     * @throws If no action is currently executing.
     */
    export function abort(): never;
}
