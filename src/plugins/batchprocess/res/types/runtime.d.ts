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

interface ScriptMethodParameterDescriptor {
    readonly name: string;

    /** Provides a brief summary, typically a single sentence. */
    readonly summary?: string;

    /** Supports GitHub Flavored Markdown. */
    readonly description?: string;

    readonly schema: import("./json-schema-2020-12").JSONSchema;

    /** Defaults to `false`. */
    readonly required?: boolean;
}

interface ScriptMethodResultDescriptor {
    readonly name: string;

    /** Provides a brief summary, typically a single sentence. */
    readonly summary?: string;

    /** Supports GitHub Flavored Markdown. */
    readonly description?: string;

    readonly schema: import("./json-schema-2020-12").JSONSchema;
}

interface ScriptMethodAnnotations {
    /** Indicates whether the operation is read-only. Defaults to `false`. */
    readonly readOnlyHint?: boolean;

    /** Indicates whether the operation may make destructive changes. Defaults to `true`. */
    readonly destructiveHint?: boolean;

    /** Indicates whether repeated calls with the same parameters are idempotent. Defaults to `false`. */
    readonly idempotentHint?: boolean;

    /** Indicates whether the operation may interact with the external world. Defaults to `true`. */
    readonly openWorldHint?: boolean;
}

interface ScriptMethodMetadata {
    /**
     * Must consist of one or more JavaScript IdentifierName segments separated by
     * dots and be unique within the containing script's methods.
     */
    readonly name: string;

    /** Provides a brief summary, typically a single sentence. */
    readonly summary?: string;

    /** Supports GitHub Flavored Markdown. */
    readonly description?: string;

    /** Must be present and may be empty. */
    readonly params: readonly ScriptMethodParameterDescriptor[];

    /** If omitted, the method may only be invoked as an RPC notification. */
    readonly result?: ScriptMethodResultDescriptor;

    readonly annotations?: ScriptMethodAnnotations;
}

/**
 * Runs synchronously on the GUI thread with an object keyed by the declared
 * parameter names without schema validation. Returns the execution result.
 * Results and logic error data use JSON serialization without schema validation.
 * A handler must not return a Promise or another thenable, which the host treats
 * as a contract violation.
 */
type ScriptMethodHandler = (params: Record<string, unknown>) => unknown;

interface ScriptMethod extends ScriptMethodMetadata {
    readonly execute: ScriptMethodHandler;
}

interface ScriptDefinition extends ScriptMetadata {
    /**
     * The host preserves declaration order and freezes this array and each of its
     * elements after validation.
     */
    readonly actions: readonly ScriptAction[];

    /**
     * The host preserves declaration order and freezes this array and each of its
     * elements after validation.
     */
    readonly methods?: readonly ScriptMethod[];
}

/**
 * Validates and registers a script definition during entry-script evaluation.
 * On success, the host freezes the definition, its actions array, its methods
 * array if present, and every action and method, then returns the original
 * object. The entry script must export that returned object.
 *
 * Invalid definitions cause a `TypeError` and are not registered. Top-level code
 * must not perform user interaction, modify documents, or access files other
 * than the script's translation files through `loadI18n`; such work belongs in
 * an action or method handler.
 *
 * @param definition The complete definition to validate and register.
 * @returns The validated and frozen input object.
 */
declare function defineScript<const T extends ScriptDefinition>(definition: T): Readonly<T>;

declare module "diffscope:runtime" {
    export type Platform = "windows" | "macos" | "linux";

    export interface Environment {
        /** The current application locale as a BCP 47 language tag. */
        readonly locale: string;

        readonly platform: Platform;
    }

    /**
     * Provides a fixed, frozen facade for global application information.
     * Properties return current values when read and are available during script
     * registration and execution without an active script context.
     */
    export const environment: Environment;

    export type ScriptActionMetadata = Omit<ScriptAction, "execute">;

    export type ScriptMethodMetadata = Omit<ScriptMethod, "execute">;

    export type ScriptMetadata = Omit<ScriptDefinition, "actions" | "methods"> & {
        /** Preserves declaration order. */
        readonly actions: readonly ScriptActionMetadata[];

        /** Preserves declaration order. */
        readonly methods: readonly ScriptMethodMetadata[];
    };

    /**
     * Represents a business logic error during RPC execution. Errors of this
     * class or its subclasses are reported as business logic errors rather than
     * internal errors.
     */
    export class MethodLogicError extends Error {
        /** Creates an error with a message and optional arbitrary data. */
        constructor(message: string, data?: unknown);

        /** RPC errors omit data if JSON serialization fails. */
        readonly data?: unknown;
    }

    export interface ScriptRuntimeBase {
        /**
         * A frozen snapshot. Its action and method arrays and their metadata
         * objects are frozen as well.
         */
        readonly metadata: Readonly<ScriptMetadata>;

        /** Indicates whether the script is registered as a built-in script. */
        readonly isBuiltin: boolean;

        /**
         * The native absolute entry-file path for a single-file script, the native
         * absolute package-directory path for a multi-file script, or an empty
         * string for a script registered with a factory.
         */
        readonly scriptPath: string;

        /**
         * The configured Scripts directory for a user single-file script, the
         * containing directory for a built-in single-file script, the package
         * directory for a multi-file script, or an empty string for a script
         * registered with a factory. File module resolution is restricted to the
         * entry file for single-file scripts and to this root directory for
         * multi-file scripts.
         */
        readonly scriptRoot: string;
    }

    export interface ActionScriptRuntime extends ScriptRuntimeBase {
        readonly kind: "action";

        /** A frozen snapshot. */
        readonly action: Readonly<ScriptActionMetadata>;

        /** The zero-based index of `action` in `metadata.actions`. */
        readonly actionIndex: number;
    }

    export interface MethodScriptRuntime extends ScriptRuntimeBase {
        readonly kind: "method";

        /** A frozen snapshot. */
        readonly method: Readonly<ScriptMethodMetadata>;

        /** The zero-based index of `method` in `metadata.methods`. */
        readonly methodIndex: number;
    }

    export type ScriptRuntime = ActionScriptRuntime | MethodScriptRuntime;

    /**
     * A fixed, frozen facade whose identity remains stable for the lifetime of an
     * engine. Each property is a getter that resolves the current execution
     * context when read.
     *
     * The facade itself may be captured during entry-script evaluation, but its
     * properties may only be read in the synchronous call chain of an executing
     * action or method. Reading a property at any other time throws an exception.
     */
    export const script: ScriptRuntime;

    /** Options accepted by the underlying i18next translate function. */
    export type TranslateOptions = Record<string, unknown>;

    /**
     * Translates a message key with the current script's loaded translation
     * resources. The function delegates to the i18next `t` function when a
     * translation file is loaded. Otherwise, or when the key is missing, it
     * returns the key.
     */
    export interface TranslateFunction {
        (key: string | readonly string[], options?: TranslateOptions): string;
        (key: string | readonly string[], defaultValue: string, options?: TranslateOptions): string;
    }

    /** The translate function for the current script's loaded resources. */
    export const t: TranslateFunction;

    /**
     * Loads translation resources for the current script from a directory of
     * JSON files named with BCP 47 language tags.
     *
     * The directory is resolved against `scriptRoot` and must remain within
     * `scriptRoot` after canonicalization. Each JSON file must contain a
     * translation object. The application locale selects the file first using
     * its preferred language list, which includes likely subtags and subtag
     * truncations. If no file matches, `fallbackLanguage` selects the file with
     * the same matching rules. If no file matches either, `t` returns message
     * keys directly.
     *
     * Only the selected language file is read. Resources belong to the current
     * script and are replaced when this function is called again for that script.
     *
     * @param directory A directory within `scriptRoot`.
     * @param fallbackLanguage A BCP 47 language tag. Defaults to `en`.
     * @throws If the directory is outside `scriptRoot`, is not a directory,
     * or the selected file is not a valid translation object.
     */
    export function loadI18n(directory: string, fallbackLanguage?: string): void;

    /**
     * Immediately interrupts the currently executing script.
     *
     * Requires an executing action or method. Throws if no handler is executing.
     * A successful call never returns to JavaScript. The host reports an action
     * failure or an internal method error and clears the interruption state when
     * execution finishes.
     *
     * @throws If no action or method is currently executing.
     */
    export function abort(): never;
}
