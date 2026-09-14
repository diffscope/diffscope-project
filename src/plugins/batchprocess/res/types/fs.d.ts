// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

/** Controlled, synchronous file-system access for DiffScope batch scripts. */
declare module "diffscope:fs" {
    export type FileAccessMode = "read" | "write" | "readWrite";

    export type FileKind = "file" | "directory" | "symbolicLink" | "other";

    export type WriteDisposition = "replace" | "createNew" | "append";

    /** A snapshot of an entry's state at the time of the call. */
    export interface FileStat {
        readonly path: string;

        /** The fully resolved absolute path, or `null` when it cannot be resolved. */
        readonly canonicalPath: string | null;

        readonly exists: boolean;

        /** Describes the entry at `path` without replacing a symbolic link with its target. */
        readonly kind: FileKind | null;

        /** Describes the final target. Broken links and missing entries produce `null`. */
        readonly resolvedKind: Exclude<FileKind, "symbolicLink"> | null;

        /** The byte length of a regular file, or `null` for other entry kinds. */
        readonly size: number | null;

        readonly createdAt: Date | null;
        readonly modifiedAt: Date | null;
        readonly accessedAt: Date | null;
        readonly metadataChangedAt: Date | null;

        /** The absolute link target, or `null` when `path` is not a symbolic link. */
        readonly symbolicLinkTarget: string | null;

        readonly hidden: boolean;

        /** Host-process readability. This does not indicate whether the script has permission. */
        readonly readable: boolean | null;

        /** Host-process writability. This does not indicate whether the script has permission. */
        readonly writable: boolean | null;

        readonly executable: boolean | null;
    }

    export interface StatOptions {
        /** Defaults to `true`. */
        readonly followSymbolicLinks?: boolean;
    }

    export interface TextReadOptions {
        /** Defaults to `utf-8`. UTF-8, UTF-16LE, and UTF-16BE are always supported. */
        readonly encoding?: string;

        /** A non-negative safe integer. The operation fails before returning oversized content. */
        readonly maxBytes?: number;
    }

    export interface BinaryReadOptions {
        /** A non-negative safe integer. The operation fails before returning oversized content. */
        readonly maxBytes?: number;
    }

    export interface TextWriteOptions {
        /** Defaults to `utf-8`. UTF-8, UTF-16LE, and UTF-16BE are always supported. */
        readonly encoding?: string;

        /** Defaults to `replace`. */
        readonly disposition?: WriteDisposition;

        /**
         * Defaults to `true` for `replace` and `false` otherwise.
         * Atomic writes are only valid with `replace` and never fall back to direct writes.
         */
        readonly atomic?: boolean;

        /**
         * Creates missing parent directories when permitted. The handle's capability scope is
         * not expanded by this option.
         */
        readonly createParents?: boolean;
    }

    export interface BinaryWriteOptions {
        /** Defaults to `replace`. */
        readonly disposition?: WriteDisposition;

        /**
         * Defaults to `true` for `replace` and `false` otherwise.
         * Atomic writes are only valid with `replace` and never fall back to direct writes.
         */
        readonly atomic?: boolean;

        readonly createParents?: boolean;
    }

    /**
     * A capability for one file path.
     *
     * A handle is bound to the engine, script action, and execution that created it. It becomes
     * invalid as soon as that action finishes, and it cannot be persisted or reused by another
     * action. Moving, removing, or changing its path to an incompatible entry kind also invalidates
     * older handles.
     */
    export interface FileHandle {
        readonly kind: "file";
        readonly path: string;
        readonly access: FileAccessMode;

        stat(options?: StatOptions): FileStat;

        /**
         * Reads and decodes the complete file.
         *
         * @throws {PermissionDeniedError} The handle does not provide read access.
         * @throws {FileSystemError} The file cannot be read or decoded.
         */
        readText(options?: TextReadOptions): string;

        /**
         * Reads the complete file into a newly allocated byte array.
         *
         * @throws {PermissionDeniedError} The handle does not provide read access.
         * @throws {FileSystemError} The file cannot be read.
         */
        readBytes(options?: BinaryReadOptions): Uint8Array;

        /**
         * Writes the complete string synchronously.
         *
         * @post On success, all encoded bytes have been committed according to `options`.
         */
        writeText(text: string, options?: TextWriteOptions): void;

        /**
         * Writes exactly the visible range of `data`. The host copies the bytes before returning.
         *
         * @post On success, all bytes have been committed according to `options`.
         */
        writeBytes(data: Uint8Array, options?: BinaryWriteOptions): void;
    }

    export interface DirectoryListOptions {
        /** Requires a directory handle whose `recursive` property is `true`. */
        readonly recursive?: boolean;

        readonly files?: boolean;
        readonly directories?: boolean;

        /** Relative-entry-path glob patterns. They do not cause additional file-system traversal. */
        readonly nameFilters?: readonly string[];
    }

    /** A frozen entry snapshot returned by `DirectoryHandle.entries()`. */
    export interface DirectoryEntry {
        /** A `/`-separated path relative to the enumerated directory. */
        readonly relativePath: string;
        readonly path: string;
        readonly kind: FileKind;
        readonly resolvedKind: Exclude<FileKind, "symbolicLink"> | null;
    }

    /**
     * A capability for a directory and, when `recursive` is true, its descendants.
     *
     * Relative child paths cannot escape through `..`, symbolic links, or junctions. Like file
     * handles, directory handles expire when the creating action finishes.
     */
    export interface DirectoryHandle {
        readonly kind: "directory";
        readonly path: string;
        readonly access: FileAccessMode;
        readonly recursive: boolean;

        stat(options?: StatOptions): FileStat;

        /** Recursive enumeration never follows symbolic-link directories. */
        entries(options?: DirectoryListOptions): readonly DirectoryEntry[];

        /**
         * Creates a capability for a child file without showing a permission dialog.
         * The requested access cannot exceed this directory capability.
         */
        file(relativePath: string, access?: FileAccessMode): FileHandle;

        /**
         * Returns a capability for an existing child directory. An empty path returns this
         * directory. Access to child directories requires a recursive directory capability.
         */
        directory(relativePath: string): DirectoryHandle;
    }

    export type ScriptPackageHandle = FileHandle | DirectoryHandle;

    export interface PermissionRequest {
        /**
         * An absolute or relative path pattern. Relative patterns use the current script root.
         * Supported glob constructs are `*`, `?`, `[]`, `[!]`, and a complete `**` path segment.
         */
        readonly pathPattern: string;

        readonly access: FileAccessMode;

        /** A non-empty, user-facing explanation shown in the permission dialog. */
        readonly reason: string;
    }

    export interface DirectoryHandleOptions {
        /** Defaults to `read`. */
        readonly access?: FileAccessMode;

        /**
         * Defaults to `false`. When enabled, existing permissions must cover the directory and
         * its complete descendant tree with the requested access mode.
         */
        readonly recursive?: boolean;
    }

    export interface CreateDirectoryOptions {
        /** Defaults to `false`. */
        readonly recursive?: boolean;
    }

    export interface CopyOptions {
        /** Defaults to `false`. */
        readonly overwrite?: boolean;

        /** Required when the source is a directory. */
        readonly recursive?: boolean;
    }

    export interface MoveOptions {
        /** Defaults to `false`. */
        readonly overwrite?: boolean;
    }

    export interface RemoveOptions {
        /** Required for permanent removal of a non-empty directory. */
        readonly recursive?: boolean;

        /**
         * Defaults to `true`. Failure to use the operating-system trash does not fall back to
         * permanent removal.
         */
        readonly useTrash?: boolean;
    }

    export interface ParsedPath {
        readonly root: string;
        readonly directory: string;
        readonly baseName: string;
        readonly name: string;
        readonly extension: string;
    }

    /**
     * Requests permission synchronously. Existing permissions that provably contain the request
     * are reused without prompting.
     *
     * The user may grant the request for this execution, for the current application session, or
     * grant this script full session access. Denial and cancellation return `false`.
     *
     * @throws {TypeError} The request shape, path pattern, access mode, or reason is invalid.
     */
    export function requestPermission(request: PermissionRequest): boolean;

    /**
     * Queries a concrete path using an existing permission.
     *
     * @throws {PermissionDeniedError} No current permission covers the path.
     */
    export function stat(path: string, options?: StatOptions): FileStat;

    /**
     * Creates a capability for a concrete file path using an existing permission. Write access
     * permits a path that does not yet exist. This call does not create or open the file.
     */
    export function getFile(path: string, access?: FileAccessMode): FileHandle;

    /** Creates a capability for an existing directory using an existing permission. */
    export function getDirectory(path: string, options?: DirectoryHandleOptions): DirectoryHandle;

    /**
     * Returns a read-only capability for the current user script package without requesting
     * permission. A built-in script has no package and receives a `notSupported` error.
     */
    export function scriptPackage(): ScriptPackageHandle;

    /**
     * Returns a recursive read-write capability for this script's persistent data directory.
     * Scripts with the same ID share this directory across actions and application sessions.
     */
    export function dataDirectory(): DirectoryHandle;

    /**
     * Returns a recursive read-write capability for a unique directory owned by the current
     * action. Repeated calls in one action return the same location. Cleanup is attempted after
     * the action finishes.
     */
    export function temporaryDirectory(): DirectoryHandle;

    /**
     * Creates a directory using existing write permissions.
     *
     * @post A successful recursive call also succeeds when the target directory already exists.
     */
    export function createDirectory(path: string, options?: CreateDirectoryOptions): void;

    /**
     * Copies an entry using read permission for the source and write permission for the
     * destination. Symbolic links are copied as links and directory links are never traversed.
     * Recursive failure does not guarantee rollback of entries already copied.
     */
    export function copy(source: string, destination: string, options?: CopyOptions): void;

    /**
     * Moves or renames an entry using write permission for both paths. Cross-device moves are not
     * emulated by copying. Existing handles for moved or replaced paths become invalid.
     */
    export function move(source: string, destination: string, options?: MoveOptions): void;

    /**
     * Removes an entry using write permission. Removing a symbolic link never removes its target.
     * Recursive permanent failure does not guarantee rollback.
     */
    export function remove(path: string, options?: RemoveOptions): void;

    /** Performs lexical normalization without accessing the file system. */
    export function normalize(path: string): string;

    /**
     * Resolves path parts without accessing the file system. If no part is absolute, the current
     * user script root is used. Built-in scripts must provide an absolute component.
     */
    export function resolve(...parts: readonly string[]): string;

    /**
     * Computes a lexical relative path without accessing the file system.
     *
     * @throws {FileSystemError} The paths belong to roots that cannot be related.
     */
    export function relative(from: string, to: string): string;

    /** Tests the current platform's absolute-path syntax without accessing the file system. */
    export function isAbsolute(path: string): boolean;

    /** Joins and lexically normalizes path parts without accessing the file system. */
    export function join(...parts: readonly string[]): string;

    export function basename(path: string): string;
    export function dirname(path: string): string;
    export function extension(path: string): string;

    /** Splits a path without accessing the file system. */
    export function parse(path: string): Readonly<ParsedPath>;

    /**
     * Formats mutually consistent path components without accessing the file system.
     *
     * @throws {FileSystemError} `baseName` does not equal `name + extension`, or the root and
     * directory cannot be combined.
     */
    export function format(path: ParsedPath): string;

    /**
     * Performs pure glob matching. Relative values use the current script root. Matching is case
     * insensitive on Windows and case sensitive on macOS and Linux.
     */
    export function matchesPattern(path: string, pattern: string): boolean;

    /**
     * Resolves an existing path through symbolic links and junctions.
     *
     * @throws {PermissionDeniedError} The lexical path or final target lacks read permission.
     */
    export function realPath(path: string): string;

    export class PermissionDeniedError extends Error {
        private constructor();

        readonly name: "PermissionDeniedError";

        /** The denied path, or `null` when no path can be reported. */
        readonly path: string | null;
    }

    export class FileSystemError extends Error {
        private constructor();

        readonly name: "FileSystemError";
        readonly operation: string;
        readonly path: string;
        readonly destinationPath: string | null;
        readonly code:
            | "notFound"
            | "alreadyExists"
            | "notFile"
            | "notDirectory"
            | "notEmpty"
            | "invalidPath"
            | "invalidState"
            | "readOnly"
            | "busy"
            | "outOfSpace"
            | "crossDevice"
            | "notSupported"
            | "tooLarge"
            | "io"
            | "encoding"
            | "unknown";
    }
}
