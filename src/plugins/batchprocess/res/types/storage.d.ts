// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

/** Synchronous application-level string storage for DiffScope batch scripts. */
declare module "diffscope:storage" {
    export type StorageErrorCode = "invalidState" | "io" | "unknown";

    /** An error reported while accessing or committing a storage change. */
    export class StorageError extends Error {
        readonly name: "StorageError";
        readonly code: StorageErrorCode;
        readonly operation: string;
        readonly storageId: string;

        /** The affected key, or `null` when the operation applies to the complete storage. */
        readonly key: string | null;

        private constructor();
    }

    /**
     * A host-owned synchronous string key-value storage.
     * Untyped JavaScript callers receive Web Storage-style `String()` conversion for keys and values.
     */
    export interface Storage {
        /** The stable ID supplied when this storage was created. */
        readonly id: string;

        readonly length: number;

        /**
         * Returns the key at a host-defined stable position in the current contents.
         *
         * @param index A non-negative safe integer.
         * @returns `null` when the index is outside the current contents.
         */
        key(index: number): string | null;

        /** Returns `null` when the key does not exist. */
        getItem(key: string): string | null;

        /**
         * Stores a value synchronously. Structured data must be serialized by the caller.
         * Persistent storage has been committed when this method returns.
         *
         * @throws {StorageError} If the host cannot commit the change. The previous value remains
         * unchanged when this happens.
         */
        setItem(key: string, value: string): void;

        /**
         * Removes a value synchronously. A missing key is ignored.
         *
         * @throws {StorageError} If the host cannot commit the change.
         */
        removeItem(key: string): void;

        /**
         * Removes every value from this storage without affecting other IDs.
         *
         * @throws {StorageError} If the host cannot commit the change.
         */
        clear(): void;
    }

    /** Opens application-level storage buckets. */
    export interface StorageService {
        /**
         * Opens a bucket retained until the application exits. Calls using the same non-empty ID
         * share one bucket across scripts, actions, and script runtime reloads.
         */
        session(id: string): Storage;

        /**
         * Opens a bucket retained in the application's dedicated script-storage file. Calls using
         * the same non-empty ID share one bucket across scripts and application runs.
         */
        persistent(id: string): Storage;
    }

    export const storage: StorageService;
}
