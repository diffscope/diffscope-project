// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

/** Synchronous access to DiffScope windows, dialogs, file pickers, and notifications. */
declare module "diffscope:shell" {
    import type { DirectoryHandle, FileHandle } from "diffscope:fs";

    /** A host-owned top-level application window. */
    export interface Window {
        /**
         * Whether the window is currently active.
         * This property safely returns `false` after the window has closed.
         */
        readonly active: boolean;

        /** Whether this handle still refers to a live host window. */
        readonly valid: boolean;

        /**
         * Shows and raises the window, then requests activation from the operating system.
         *
         * @throws {WindowError} With code `invalidWindow` if the window has closed.
         * @post The activation request has been submitted synchronously; the operating system may still deny focus.
         */
        raise(): void;
    }

    export type WindowErrorCode = "invalidWindow" | "unsupportedOperation";

    /** An error caused by an invalid window or an operation unavailable in the selected window context. */
    export class WindowError extends Error {
        readonly name: "WindowError";
        readonly code: WindowErrorCode;

        /** The explicit window associated with the failure, or `null` when no window was selected. */
        readonly window: Window | null;

        private constructor();
    }

    export type MessageSeverity = "information" | "success" | "warning" | "critical" | "question";

    /** Determines a custom button's platform ordering and semantic role. */
    export type MessageButtonRole = "accept" | "reject" | "destructive" | "help" | "action";

    export interface MessageButton {
        /** A stable ID returned by `dialogs.message()`. IDs must be unique within the message. */
        readonly id: string;
        readonly text: string;

        /** Defaults to `action`. */
        readonly role?: MessageButtonRole;

        /** At most one button may be the default button. */
        readonly default?: boolean;

        /** At most one button may be activated by Escape or a window-close request. */
        readonly escape?: boolean;
    }

    export interface MessageDialogOptions {
        readonly title: string;
        readonly message: string;

        /** Additional text shown in the expandable details area. */
        readonly detail?: string;

        /** Defaults to `information`. */
        readonly severity?: MessageSeverity;

        /** A localized OK button with ID `ok` is used when this property is omitted. */
        readonly buttons?: readonly [MessageButton, ...MessageButton[]];

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface AlertDialogOptions {
        /** Defaults to the current script name. */
        readonly title?: string;

        /** Defaults to `information`. */
        readonly severity?: Exclude<MessageSeverity, "question">;

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface ConfirmDialogOptions {
        /** Defaults to the current script name. */
        readonly title?: string;

        /** Defaults to the platform's localized OK text. */
        readonly acceptText?: string;

        /** Defaults to the platform's localized Cancel text. */
        readonly cancelText?: string;

        /** Uses destructive-button semantics and warning visuals when `true`. */
        readonly destructive?: boolean;

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface TextPromptOptions {
        readonly label: string;

        /**
         * When omitted for a single-line prompt, the compact Quick Input UI is used.
         * Providing a title or requesting multiline input uses a dialog. A dialog without an explicit title
         * uses the current script name.
         */
        readonly title?: string;

        /** Defaults to an empty string. */
        readonly value?: string;
        readonly placeholder?: string;

        /** Defaults to `false`. */
        readonly multiline?: boolean;

        /**
         * Defaults to `true`. When `false`, input containing only whitespace cannot be submitted.
         * The returned value is the validated original text and is not trimmed.
         */
        readonly allowEmpty?: boolean;

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface ChoiceItem {
        /** Must be unique among the items in the same choice list. */
        readonly value: string;
        readonly label: string;
        readonly description?: string;

        /** Defaults to `true`. */
        readonly enabled?: boolean;
    }

    export interface ChoiceDialogOptions {
        /** The picker prompt. */
        readonly title: string;

        /** Additional search guidance displayed with the picker prompt. */
        readonly placeholder?: string;
        readonly items: readonly ChoiceItem[];

        /** Selects the matching enabled item; otherwise the first enabled item is selected. */
        readonly initialValue?: string;

        /** Initial case-insensitive filter text. */
        readonly initialFilter?: string;

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface FormFieldBase {
        /** The result key. Names must be unique within a form. */
        readonly name: string;
        readonly label: string;
        readonly description?: string;

        /** Defaults to `false`. Its exact constraint depends on the field kind. */
        readonly required?: boolean;
    }

    export interface TextFormField extends FormFieldBase {
        readonly type: "text";
        readonly value?: string;
        readonly placeholder?: string;

        /** Defaults to `false`. */
        readonly multiline?: boolean;

        /** A non-negative safe integer. Defaults to `0`. */
        readonly minLength?: number;

        /** A safe integer no smaller than `minLength`. When omitted, no maximum is imposed. */
        readonly maxLength?: number;

        /** An ECMAScript regular-expression source that must match the entire value. */
        readonly pattern?: string;
    }

    export interface IntegerFormField extends FormFieldBase {
        readonly type: "integer";

        /** A safe integer within the effective range. Defaults to zero clamped to that range. */
        readonly value?: number;

        /** A safe integer. Defaults to `0`. */
        readonly minimum?: number;

        /** A safe integer no smaller than `minimum`. Defaults to `100`. */
        readonly maximum?: number;

        /** A positive safe integer. Defaults to `1`. */
        readonly step?: number;
    }

    export interface NumberFormField extends FormFieldBase {
        readonly type: "number";

        /** A finite number within the effective range. Defaults to zero clamped to that range. */
        readonly value?: number;

        /** A finite number. Defaults to `0`. */
        readonly minimum?: number;

        /** A finite number no smaller than `minimum`. Defaults to `100`. */
        readonly maximum?: number;

        /** A positive finite number. Defaults to `0.01`. */
        readonly step?: number;

        /** A safe integer from 0 through 15. Defaults to `2`. */
        readonly decimals?: number;
    }

    export interface BooleanFormField extends FormFieldBase {
        readonly type: "boolean";

        /** Defaults to `false`. */
        readonly value?: boolean;
    }

    export interface ChoiceFormField extends FormFieldBase {
        readonly type: "choice";
        readonly items: readonly ChoiceItem[];

        /**
         * Selects the matching enabled item. Otherwise, the first enabled item is selected.
         * A required choice field must contain at least one enabled item.
         */
        readonly value?: string;
    }

    export interface Color {
        /** A finite value from 0 through 1. */
        readonly red: number;
        /** A finite value from 0 through 1. */
        readonly green: number;
        /** A finite value from 0 through 1. */
        readonly blue: number;
        /** A finite value from 0 through 1. */
        readonly alpha: number;

        /** A normalized CSS representation generated by the host. */
        readonly css: string;
    }

    export interface ColorFormField extends FormFieldBase {
        readonly type: "color";

        /** Defaults to opaque black. CSS strings are not accepted. */
        readonly value?: Color;

        /**
         * Whether Alpha can be edited. Defaults to `false`; a valid initial Alpha value is preserved when hidden.
         */
        readonly alpha?: boolean;
    }

    export type FormField = TextFormField | IntegerFormField | NumberFormField | BooleanFormField | ChoiceFormField | ColorFormField;

    export type FormValue = string | number | boolean | Color;

    export type FormFieldValue<TField extends FormField> =
        TField extends TextFormField ? string
            : TField extends IntegerFormField | NumberFormField ? number
                : TField extends BooleanFormField ? boolean
                    : TField extends ChoiceFormField ? string
                        : TField extends ColorFormField ? Color
                            : never;

    export type FormResult<TFields extends readonly FormField[]> = Readonly<{
        [TField in TFields[number] as TField["name"]]: FormFieldValue<TField>;
    }>;

    export interface FormDialogOptions<TFields extends readonly FormField[] = readonly FormField[]> {
        readonly title: string;
        readonly description?: string;
        readonly fields: TFields;

        /** Defaults to the platform's localized OK text. */
        readonly acceptText?: string;

        /** Defaults to the platform's localized Cancel text. */
        readonly cancelText?: string;

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface FileFilter {
        readonly name: string;

        /** A non-empty list of platform file-dialog patterns, such as `*.json`. */
        readonly patterns: readonly [string, ...string[]];
    }

    export interface OpenFileDialogOptions {
        /** Defaults to localized host text. */
        readonly title?: string;

        /** Used only for initial navigation and does not itself grant file-system access. */
        readonly initialPath?: string;

        readonly filters?: readonly FileFilter[];

        /** Defaults to `false`. */
        readonly multiple?: boolean;

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface SaveFileDialogOptions {
        /** Defaults to localized host text. */
        readonly title?: string;
        readonly initialPath?: string;

        /** Appended when `initialPath` is empty or identifies a directory. */
        readonly suggestedName?: string;

        readonly filters?: readonly FileFilter[];

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface SelectDirectoryDialogOptions {
        /** Defaults to localized host text. */
        readonly title?: string;
        readonly initialPath?: string;

        /** Whether the returned capability covers descendant paths. Defaults to `false`. */
        readonly recursive?: boolean;

        /** Defaults to `read`. */
        readonly access?: "read" | "write" | "readWrite";

        /**
         * The owning application window. When omitted or `null`, the active application window is used,
         * followed by the first available application window.
         */
        readonly parent?: Window | null;
    }

    export interface DialogService {
        /**
         * Displays a modal message and blocks until a button is chosen or the window closes.
         *
         * @returns The selected button ID, or `null` when the window closes without an escape button.
         * @throws {TypeError} If the options or custom buttons are invalid.
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         */
        message(options: MessageDialogOptions): string | null;

        /**
         * Displays a modal one-button message.
         *
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         */
        alert(message: string, options?: AlertDialogOptions): void;

        /**
         * Displays a modal confirmation.
         *
         * @returns `true` only when the accept button is chosen.
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         */
        confirm(message: string, options?: ConfirmDialogOptions): boolean;

        /**
         * Prompts for validated text.
         *
         * @returns The submitted text, or `null` when canceled.
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         */
        promptText(options: TextPromptOptions): string | null;

        /**
         * Displays a searchable single-choice picker.
         *
         * @returns The selected item's value, or `null` when canceled or when no item is enabled.
         * @throws {TypeError} If the options contain invalid or duplicate values.
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         */
        choose(options: ChoiceDialogOptions): string | null;

        /**
         * Displays a schema-defined form. Text constraints are checked before submission.
         *
         * @returns A frozen result keyed by field name, or `null` when canceled.
         * @throws {TypeError} If the schema or an initial value is invalid.
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         */
        form<const TFields extends readonly FormField[]>(options: FormDialogOptions<TFields>): FormResult<TFields> | null;

        /**
         * Lets the user select one or more existing files.
         *
         * @returns Action-scoped read capabilities, or `null` when canceled.
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         * @post Each returned handle is authorized only for the current action execution.
         */
        openFile(options?: OpenFileDialogOptions & { readonly multiple?: false }): FileHandle | null;
        openFile(options: OpenFileDialogOptions & { readonly multiple: true }): readonly FileHandle[] | null;

        /**
         * Lets the user select a destination file, which need not exist yet.
         *
         * @returns An action-scoped write capability, or `null` when canceled.
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         * @post The returned handle is authorized only for the current action execution.
         */
        saveFile(options?: SaveFileDialogOptions): FileHandle | null;

        /**
         * Lets the user select an existing directory.
         *
         * @returns An action-scoped directory capability, or `null` when canceled.
         * @throws {WindowError} If the selected window is invalid or no application window is available.
         * @post The returned handle is authorized only for the current action execution.
         */
        selectDirectory(options?: SelectDirectoryDialogOptions): DirectoryHandle | null;
    }

    export const dialogs: Readonly<DialogService>;

    export type NotificationSeverity = Exclude<MessageSeverity, "question">;
    export type NotificationBubbleMode = "normal" | "hidden" | "autoHide";

    export interface NotificationOptions {
        readonly title: string;
        readonly message: string;

        /** Defaults to `information`. */
        readonly severity?: NotificationSeverity;

        /** Defaults to `normal`. */
        readonly bubble?: NotificationBubbleMode;

        /**
         * Sends to the selected Project window. Home windows do not support window notifications.
         * When omitted or `null`, the notification is sent to the application-wide notification center.
         */
        readonly window?: Window | null;
    }

    export interface NotificationService {
        /**
         * Enqueues a notification without waiting for it to be painted.
         *
         * @throws {WindowError} With code `invalidWindow` for a closed target, or `unsupportedOperation`
         * when the target window has no window-scoped notification center.
         * @post The notification has been queued when this method returns.
         */
        post(options: NotificationOptions): void;
    }

    export const notifications: Readonly<NotificationService>;
}
