// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

/// <reference lib="es2015.iterable" />

/**
 * Synchronous application, document, and model access for batch script actions.
 *
 * Service properties and host-owned objects are accessible only in the synchronous
 * call chain of an executing action. Importing a service facade is permitted during
 * entry-script evaluation. The associated project is resolved for each action.
 *
 * Host-owned objects cannot be constructed by scripts. Every document mutation
 * requires an active transaction owned by the executing action for that document.
 * Selection and playhead changes do not require a transaction and are not undoable.
 * Methods documented as creating their own undo step require no active transaction.
 * Writes are visible immediately, before commit. A failed individual mutation must
 * leave its target unchanged. Previously successful mutations remain in the transaction.
 *
 * All numeric inputs must be finite unless otherwise documented. Integer model
 * fields use signed 32-bit integers. Non-negative ticks are from 0 through
 * 2147483647. Coordinate arithmetic must remain within the representable range.
 * Persisted tick positions and durations are non-negative. Loop duration is positive.
 * Model gain uses non-negative linear amplitude, with unity gain equal to 1.
 * Model pan is from -1 through 1. Indices and counts are non-negative integers.
 * A splice index is at most the array length and deleteCount must fit the remaining range.
 * A rotation requires 0 <= leftIndex <= middleIndex <= rightIndex <= length.
 * It swaps the adjacent ranges [leftIndex, middleIndex) and [middleIndex, rightIndex).
 * Invalid argument types cause TypeError. Invalid numeric ranges cause RangeError.
 * Invalid host objects, transaction conflicts, and invalid model data cause CoreError.
 * File permissions are enforced through diffscope:fs.
 * Init objects contain ordinary data only. Their fields use the same units and
 * constraints as the corresponding model properties. Creation and DSPX import
 * copy input data and validate the complete supplied tree before mutation.
 * Sequence insertions and replacements preserve their documented uniqueness
 * constraints. A conflicting position or key causes RangeError.
 * createFromOpenDSPX methods import one complete entity tree into the receiving
 * container. They require a script transaction and leave the container unchanged
 * on failure. Omitted insertion indices append to ordered lists.
 * Creation initializes and attaches the complete entity tree within one call.
 * Deletion destroys the entity tree. Detached entities are not exposed to scripts.
 * Uncommitted script transactions are aborted when an action ends.
 */
declare module "diffscope:core" {
    import type * as DSPX from "diffscope:dspx";
    import type { FileHandle } from "diffscope:fs";
    import type { Color, Window } from "diffscope:shell";
    import type { Storage } from "diffscope:storage";

    export type JsonValue = DSPX.JsonValue;
    export type Tick = number;
    /** Represents a non-negative, zero-based measure index. */
    export type MeasureIndex = number;
    /** Preserves the full native entity ID as an opaque string valid within its document. */
    export type Handle = string;

    export type CoreErrorCode = "invalidState" | "invalidObject" | "transactionConflict" | "invalidData" | "unsupportedOperation" | "unknown";

    export class CoreError extends Error {
        readonly name: "CoreError";
        readonly code: CoreErrorCode;
        readonly operation: string;
        readonly project: Project | null;
        private constructor();
    }

    /**
     * Exports independently mutable plain data through the native OpenDSPX conversion
     * mechanism, including registered conversion delegates where applicable.
     */
    export interface OpenDSPXConvertible<T> {
        /**
         * Returns a detached snapshot of the current state, including uncommitted writes.
         * Returned data uses DSPX field names, decibel gain, and segmented parameter curves.
         * Native conversion may normalize representation. Export does not commit or save.
         *
         * @throws {CoreError} With code invalidData if the current state cannot be
         * represented as valid DSPX JSON, including non-finite decibel gain at zero
         * linear gain. Use mute with a positive linear gain for serializable silence.
         */
        toOpenDSPX(): T;
    }

    export interface OpenDSPXEditable<T> extends OpenDSPXConvertible<T> {
        /**
         * Replaces the complete contents through the native OpenDSPX conversion
         * mechanism. Requires the executing action's transaction for this document.
         * Validates the entire input before mutation. Failure leaves the target unchanged.
         * Retains the target identity and parent membership. Sequence order follows
         * imported time coordinates. Replaced child entities become invalid and are
         * removed from selection. Collection wrappers retain their owner's lifetime.
         * A clip's or singer's discriminant must match the target's kind.
         * Serialized time and other data fields replace their corresponding model fields.
         *
         * Input data is copied. No references to caller-owned objects are retained.
         * Absent optional workspace extensions reset their mapped properties to
         * model defaults. Every mapped property receives an imported value or a default.
         * Persisted changes are undoable. Original pronunciation, phoneme, and
         * parameter caches are transient and may be regenerated independently of history.
         * Invalid input is rejected before mutation.
         */
        fromOpenDSPX(data: T): void;
    }

    export interface SingerLanguageDescriptor {
        /** The ID follows registry language identifiers without BCP 47 normalization. */
        readonly id: string;
        readonly name: string;
        readonly defaultLyric: string;
    }

    export type ParameterValueType = "absolute" | "relative";
    export type ParameterFillMode = "none" | "top" | "bottom" | "baseline";

    /** Separates DSPX integers, normalized values, and architecture-specific display values. */
    export interface ParameterDescriptor {
        readonly id: string;
        readonly transform: boolean;
        /** The display name may be empty if none was supplied. */
        readonly displayName: string;
        /** The fallback value uses the normalized domain from 0 through 1. */
        readonly defaultValue: number;
        /** The reference line and fill baseline use this normalized value. */
        readonly baselineValue: number;
        readonly valueType: ParameterValueType;
        readonly fillMode: ParameterFillMode;
        readonly showDefaultValue: boolean;

        /** Returns clamp(value / 8064000, 0, 1), or null for an undefined sample. */
        fromDSPXValue(value: number | null): number | null;
        /**
         * Returns round(clamp(value, 0, 1) * 8064000).
         * Non-finite inputs use the normalized defaultValue.
         */
        toDSPXValue(value: number): number;
        /** Returns the display value for a clamped normalized value, or null for null or non-finite input. */
        toDisplayValue(value: number | null): number | null;
        /** Returns the clamped normalized value, or null if the architecture conversion is non-finite. */
        fromDisplayValue(value: number): number | null;
        /** Returns the localized display string for a normalized value, or an empty string for null or non-finite input. */
        formatValue(value: number | null): string;
    }

    export interface SingerArchitectureDescriptor {
        /** Matches Sources.category. */
        readonly id: string;
        readonly name: string;
        /** Registered architecture parameters are sorted by ID. Built-in pitch is resolved separately. */
        readonly parameters: readonly ParameterDescriptor[];
        /** The value contains architecture metadata. Sources has no corresponding extra field. */
        readonly defaultExtra: JsonValue;
    }

    export interface SingerDescriptor {
        readonly architectureId: string;
        readonly singerId: string;
        readonly name: string;
        readonly avatarUrl: string | null;
        readonly backgroundUrl: string | null;
        /** The registered default language may be empty. */
        readonly defaultLanguage: string;
        /** The registered languages are sorted by ID without synthetic fallback entries. */
        readonly languages: readonly SingerLanguageDescriptor[];
        /** The list contains singer-specific parameter IDs. Built-in pitch support is independent. */
        readonly supportedParameters: readonly string[];
        /** A common non-empty group is required for the application's singer mixing workflow. */
        readonly mixGroup: string;
        readonly defaultExtra: JsonValue;
    }

    /** Reads registry snapshots. Scripts cannot register or remove architectures or singers. */
    export interface SingerRegistry {
        /** Returns architecture snapshots sorted by ID. */
        architectures(): readonly SingerArchitectureDescriptor[];
        /** Returns null for an unregistered architecture. */
        architecture(architectureId: string): SingerArchitectureDescriptor | null;
        /** Returns singers sorted by ID, or an empty array for an unregistered architecture. */
        singers(architectureId: string): readonly SingerDescriptor[];
        /** Returns null for an unregistered singer. */
        singer(architectureId: string, singerId: string): SingerDescriptor | null;
        /**
         * Returns a parameter descriptor, or null if unavailable.
         * Built-in pitch is available even without a registered architecture.
         * Transform descriptors use the shared relative transform definition,
         * with normalized default and baseline 0.5 and display range 0 through 2.
         * Other descriptors require an architecture parameter with the supplied ID.
         * This lookup does not imply support by a particular singer.
         */
        parameter(architectureId: string, parameterId: string, transform?: boolean): ParameterDescriptor | null;
    }

    export const singers: SingerRegistry;

    export interface LyricDefaults {
        /**
         * Returns the default lyric using the first non-empty single-singer ID in
         * depth-first list order. Uses its default language, or first registry
         * language if the default language is empty. Missing singer, language, or
         * lyric data uses the application's configured fallback lyric.
         */
        lyricFor(clip: SingingClip): string;
    }

    export const lyricDefaults: LyricDefaults;

    export interface TrackColorPalette {
        /** The array contains a snapshot of the current palette. Track.colorId indexes this array. */
        readonly colors: readonly Color[];
    }

    export const trackColors: TrackColorPalette;

    export interface NewProjectOptions {
        /** Uses the application default if absent or empty. */
        readonly defaultName?: string;
        /** Uses the application template if absent. */
        readonly model?: DspxModelInit;
    }

    export interface Application {
        /**
         * The project associated with the executing action, or null for a home-window
         * action. Fixed for that invocation, even if another project becomes active.
         * The reference may become closed during the action. requiresProject ensures
         * an open associated project when invocation begins.
         */
        readonly project: Project | null;
        readonly projects: readonly Project[];
        /** Resolves the current home window on each read. */
        readonly homeWindow: Window | null;
        /** Returns an initialized project, or null if initialization fails. */
        newProject(options?: NewProjectOptions): Project | null;
        /** Creates a project from validated DSPX data. Returns null if project initialization fails. */
        newProjectFromOpenDSPX(data: DSPX.Model, options?: Omit<NewProjectOptions, "model">): Project | null;
        /** Returns a new project, or null on cancellation, rejected validation, or loading failure. */
        newProjectFromTemplate(templateFile: FileHandle, options?: Omit<NewProjectOptions, "model">): Project | null;
        /**
         * Opens a readable DSPX file, or displays a picker if file is omitted.
         * Returns the existing project and raises its window if the file is already
         * open. Returns null on cancellation, rejected validation, or loading failure.
         */
        openProject(file?: FileHandle): Project | null;
        /** Shows or raises the home window. */
        showHome(): void;
    }

    export const application: Application;

    export interface ProjectStorageService {
        /**
         * Opens a string storage bucket shared by the same project and non-empty ID.
         * Retained until project closure. Contents are not written into DSPX data.
         */
        session(id: string): Storage;
    }

    export interface Project {
        /** The ID remains stable within the application run. */
        readonly id: string;
        readonly window: Window;
        readonly storage: ProjectStorageService;
        readonly displayName: string;
        /** The path is absolute, or null before the first save. Reading it grants no file access. */
        readonly filePath: string | null;
        /** Compares committed history with the clean position. Uncommitted writes are excluded. */
        readonly modified: boolean;
        /** Remains readable after closure. Other members then throw CoreError with code invalidObject. */
        readonly closed: boolean;
        readonly document: Document;
        readonly timeline: Timeline;
        /** Saves to the current file, or invokes saveAs() if unsaved. Requires no active transaction. */
        save(): boolean;
        /** Saves to a writable target or a picker selection. Returns false on cancellation or failure. Requires no active transaction. */
        saveAs(target?: FileHandle): boolean;
        /** Saves a copy without changing filePath or modified. Returns false on cancellation or failure. Requires no active transaction. */
        saveCopy(target?: FileHandle): boolean;
        /**
         * Requests closure through the normal save prompt. Returns true if closed,
         * or false if cancellation prevents closure. Requires no active transaction.
         * Project-owned objects become invalid on successful closure.
         */
        close(): boolean;
        /**
         * Triggers an enabled action in the window's QActionKit action context.
         * Returns false for unknown or disabled IDs. Returns true if dispatched,
         * without guaranteeing the action's result. Requires no active script transaction.
         */
        triggerAction(actionId: string): boolean;
    }

    export interface MusicalPosition {
        readonly measure: MeasureIndex;
        /** The beat index is zero-based within the measure. */
        readonly beat: number;
        /** The tick offset is non-negative within the beat. */
        readonly tick: number;
    }

    export interface TimeSignatureValue {
        /** The numerator must be an integer from 1 through 1118481. */
        readonly numerator: number;
        readonly denominator: DSPX.TimeSignatureDenominator;
    }

    export interface Timeline {
        readonly ticksPerQuarter: 480;
        /** The playhead is from 0 through 2147483646. Assignment does not start playback. */
        playhead: Tick;
        /** The previous playhead is from 0 through 2147483646. */
        previousPlayhead: Tick;
        /** The non-negative range hint does not restrict document entity positions. */
        contentEndHint: Tick;
        /** Sets both playheads to a tick from 0 through 2147483646. */
        goTo(tick: Tick): void;
        /** Returns the zero-based musical position of a non-negative project tick. */
        toMusicalPosition(tick: Tick): MusicalPosition;
        /** Returns a project tick, normalizing excess non-negative beat and tick offsets. */
        toTick(position: MusicalPosition): Tick;
        /** Returns milliseconds from project start for a non-negative tick. */
        toMilliseconds(tick: Tick): number;
        /** Returns the nearest tick for non-negative milliseconds. */
        fromMilliseconds(milliseconds: number): Tick;
        /** Returns the tempo effective at the project tick. */
        tempoAt(tick: Tick): number;
        /** Returns the time signature effective at the measure. */
        timeSignatureAt(measure: MeasureIndex): TimeSignatureValue;
    }

    export interface Transaction {
        /** Remains readable after completion. */
        readonly active: boolean;
        /**
         * Commits an undo step with a non-blank, localized user-facing name.
         * Call abort() for a no-op. The native controller does not detect empty steps.
         * Other calls after completion throw CoreError with code invalidState.
         */
        commit(name: string): void;
        /** Restores persisted model state and completes the transaction. Transient caches may be regenerated. */
        abort(): void;
    }

    export interface UndoHistory {
        readonly position: number;
        readonly length: number;
        /** The saved history position is -1 if a history branch made it unreachable. */
        readonly cleanPosition: number;
        readonly canUndo: boolean;
        readonly canRedo: boolean;
        readonly undoName: string | null;
        readonly redoName: string | null;
        /** Returns whether a step was undone. Requires no active transaction. */
        undo(): boolean;
        /** Returns whether a step was redone. Requires no active transaction. */
        redo(): boolean;
        /** Moves to an integer position from 0 through length. Requires no active transaction. */
        goTo(position: number): void;
    }

    export type SelectionType = "anchorNode" | "clip" | "dynamicMixingAnchor" | "keySignature" | "label" | "note" | "tempo" | "track";
    export type SelectionKind = SelectionType | "freeParameter";

    export interface SelectionEntityMap {
        readonly anchorNode: AnchorNode;
        readonly clip: Clip;
        readonly dynamicMixingAnchor: DynamicMixingAnchor;
        readonly keySignature: KeySignature;
        readonly label: Label;
        readonly note: Note;
        readonly tempo: Tempo;
        readonly track: Track;
    }

    export type SelectableEntity = SelectionEntityMap[SelectionType];

    /** Provides a live typed view. The current focus item need not belong to the selected set. */
    export interface EntitySelectionView<T extends SelectionType> {
        readonly type: T;
        readonly active: boolean;
        readonly current: SelectionEntityMap[T] | null;
        /** The array contains a snapshot with unspecified order. */
        readonly items: readonly SelectionEntityMap[T][];
        readonly count: number;
        /** Returns false for invalid items or items from another document. */
        has(item: SelectionEntityMap[T]): boolean;
    }

    export interface AnchorNodeSelectionView extends EntitySelectionView<"anchorNode"> {
        /** The sequence context may remain set for an empty selection. */
        readonly context: AnchorNodeSequence | null;
    }

    export interface NoteSelectionView extends EntitySelectionView<"note"> {
        readonly context: NoteSequence | null;
    }

    export interface DynamicMixingAnchorSelectionView extends EntitySelectionView<"dynamicMixingAnchor"> {
        readonly context: DynamicMixingAnchorSequence | null;
    }

    export interface ClipSelectionView extends EntitySelectionView<"clip"> {
        readonly singingClipCount: number;
        readonly audioClipCount: number;
        readonly sequences: readonly ClipSequence[];
    }

    export interface SelectionViewMap {
        readonly anchorNode: AnchorNodeSelectionView;
        readonly clip: ClipSelectionView;
        readonly dynamicMixingAnchor: DynamicMixingAnchorSelectionView;
        readonly keySignature: EntitySelectionView<"keySignature">;
        readonly label: EntitySelectionView<"label">;
        readonly note: NoteSelectionView;
        readonly tempo: EntitySelectionView<"tempo">;
        readonly track: EntitySelectionView<"track">;
    }

    export type AnySelectionView = SelectionViewMap[SelectionType];

    export interface SelectOptions {
        /** Defaults to false. A type or constrained-sequence switch always clears the previous set. */
        readonly clear?: boolean;
        /** Defaults to true. */
        readonly current?: boolean;
    }

    export interface FreeParameterSelection {
        readonly active: boolean;
        readonly clip: SingingClip | null;
        readonly parameterId: string | null;
        readonly displayName: string | null;
        readonly layer: "edited" | "transform" | null;
        readonly hasRange: boolean;
        /** The range uses clip-local content ticks in [start, end), or null without a range. */
        readonly start: Tick | null;
        readonly end: Tick | null;
        /**
         * Binds an attached clip and non-empty parameter ID. The parameter need not
         * exist yet. Clears entity selection. A changed context clears the range.
         * An omitted or empty displayName uses parameterId.
         */
        setContext(clip: SingingClip, parameterId: string, layer: "edited" | "transform", displayName?: string): void;
        /** Sets a non-negative half-open range. Equal endpoints clear it. Requires a context. */
        setRange(start: Tick, end: Tick): void;
        /** Clears the range and retains the context. */
        clearRange(): void;
        /** Clears both range and context. */
        clearContext(): void;
    }

    export interface Selection {
        readonly type: SelectionType | null;
        readonly kind: SelectionKind | null;
        readonly current: SelectableEntity | null;
        readonly count: number;
        readonly activeView: AnySelectionView | null;
        readonly anchorNodes: AnchorNodeSelectionView;
        readonly clips: ClipSelectionView;
        readonly dynamicMixingAnchors: DynamicMixingAnchorSelectionView;
        readonly keySignatures: EntitySelectionView<"keySignature">;
        readonly labels: EntitySelectionView<"label">;
        readonly notes: NoteSelectionView;
        readonly tempos: EntitySelectionView<"tempo">;
        readonly tracks: EntitySelectionView<"track">;
        /** This context is mutually exclusive with an active entity selection context. */
        readonly freeParameter: FreeParameterSelection;
        /** Returns the typed view even if inactive. */
        view<T extends SelectionType>(type: T): SelectionViewMap[T];
        /** Returns a snapshot of the requested typed selection. */
        items<T extends SelectionType>(type: T): readonly SelectionEntityMap[T][];
        /** Returns the typed current item, which may be retained while inactive. */
        currentItem<T extends SelectionType>(type: T): SelectionEntityMap[T] | null;
        /** Returns whether the active typed selection contains the item. */
        has(item: SelectableEntity): boolean;
        /**
         * Selects an attached item from this document. Switches type and sequence
         * context as required. Clears a free-parameter context when activated.
         */
        select(item: SelectableEntity, options?: SelectOptions): void;
        /** Removes an item from selection. Does not change an unrelated selection context. */
        deselect(item: SelectableEntity): void;
        /** Toggles selection and sets the current item. */
        toggle(item: SelectableEntity): void;
        /** Activates the item's type and context without selecting the item. */
        setCurrent(item: SelectableEntity): void;
        /** Clears the current item. An explicit type activates its selection context. */
        clearCurrent(type?: SelectionType): void;
        /** Clears the active selected set and current item. An inactive explicit type is ignored. */
        clear(type?: SelectionType): void;
        /** Establishes a sequence context that permits an empty selection. */
        setContext(type: "note", context: NoteSequence): void;
        setContext(type: "anchorNode", context: AnchorNodeSequence): void;
        setContext(type: "dynamicMixingAnchor", context: DynamicMixingAnchorSequence): void;
        /** Selects every eligible item within the supplied sequence or document scope. */
        selectAll(type: "note", context: NoteSequence): void;
        selectAll(type: "anchorNode", context: AnchorNodeSequence): void;
        selectAll(type: "dynamicMixingAnchor", context: DynamicMixingAnchorSequence): void;
        selectAll(type: Exclude<SelectionType, "note" | "anchorNode" | "dynamicMixingAnchor">): void;
    }

    export type PasteMode = "currentPosition" | "relativeToCopiedPlayhead" | "originalPosition";

    /** Uses the system DSPX clipboard. Operations require no active transaction. */
    export interface DocumentClipboard {
        readonly canCopy: boolean;
        readonly canPaste: boolean;
        /** Copies current selection using the project playhead if referenceTick is omitted. */
        copy(referenceTick?: Tick): void;
        /** Copies and deletes current selection through a named undo step. */
        cut(referenceTick?: Tick): void;
        /**
         * Pastes through a named undo step. Defaults to currentPosition and the
         * project playhead. Relative mode offsets from the copied playhead.
         * Original mode retains copied positions and, for clips, original tracks.
         * Parameter paste may display a resampling confirmation. Cancellation is a no-op.
         */
        paste(referenceTick?: Tick, mode?: PasteMode): void;
    }

    export interface Document extends OpenDSPXConvertible<DSPX.Model> {
        readonly model: DspxModel;
        readonly selection: Selection;
        readonly history: UndoHistory;
        readonly clipboard: DocumentClipboard;
        /** Includes host transactions, including non-exclusive transactions. */
        readonly transactionActive: boolean;
        /**
         * Begins an exclusive transaction. An existing exclusive transaction causes
         * CoreError with code transactionConflict. A pending non-exclusive host
         * transaction is committed first through TransactionController.
         * Any script transaction still active when the action returns, throws, or
         * is interrupted is aborted. Previously committed steps remain committed.
         */
        beginTransaction(): Transaction;
        /** Deletes selected entities or a free-parameter range through a named undo step. */
        deleteSelection(): void;
        /**
         * Splits selected clips or notes strictly crossing tick through a named
         * undo step. Uses project ticks for clip selection and clip-local content
         * ticks for note selection, matching DspxDocument::splitItems().
         */
        splitSelectionAt(tick: Tick): void;
        /** Includes singer-layout, overlap, and parameter-resampling restrictions. */
        readonly canBounceSelection: boolean;
        /** Compares singer layouts separately within each selected track. False without eligible clips. */
        readonly selectedClipSingerLayoutIdentical: boolean;
        /** Merges selected singing clips by track through a named undo step. Requires canBounceSelection. */
        bounceSelectionToClip(): void;
    }

    export type EntityData = DSPX.Track | DSPX.Clip | DSPX.Note | DSPX.Phoneme | DSPX.Param | DSPX.AnchorNode | DSPX.Sources | DSPX.Singer | DSPX.DynamicMixingAnchor | DSPX.Tempo | DSPX.TimeSignature | DSPX.Label | DSPX.KeySignature | DSPX.AudioDSP;

    export interface Entity<T extends EntityData = EntityData> extends OpenDSPXEditable<T> {
        /** Remains readable after invalidation. Does not identify an entity across documents or application runs. */
        readonly handle: Handle;
        readonly model: DspxModel;
        /**
         * Remains readable after deletion or project closure. Members other than
         * valid and handle then throw CoreError with code invalidObject. Undo
         * restoration does not make an invalidated wrapper usable again.
         * Reacquire restored entities.
         */
        readonly valid: boolean;
    }

    /**
     * Iterators capture an ordered snapshot when created. Subsequent mutations do
     * not change that snapshot, but deleting a captured entity invalidates its wrapper.
     * Every mutating collection operation requires the document's script transaction.
     */
    export interface EntityCollection<T extends Entity> extends Iterable<T> {
        readonly length: number;
        readonly first: T | null;
        readonly last: T | null;
        /** Returns a snapshot in current collection order. */
        toArray(): readonly T[];
        /** Returns false for invalid items or items from another model. */
        has(item: T): boolean;
    }

    /** All writable properties require the document's script transaction. */
    export interface DspxModel extends OpenDSPXEditable<DSPX.Model> {
        readonly document: Document;
        projectName: string;
        projectAuthor: string;
        /** The global shift is an integer from -50 through 50, in cents. */
        globalCentShift: number;
        /** Routes tracks to separate stereo pairs if true. */
        multiChannelOutput: boolean;
        /** The linear amplitude gain must be non-negative. Unity gain is 1. */
        gain: number;
        /** The pan must be from -1 through 1. */
        pan: number;
        mute: boolean;
        loopEnabled: boolean;
        loopStart: Tick;
        /** The loop length must be positive, in ticks. */
        loopLength: Tick;
        readonly labels: LabelSequence;
        readonly keySignatures: KeySignatureSequence;
        readonly tempos: TempoSequence;
        readonly timeSignatures: TimeSignatureSequence;
        readonly tracks: TrackList;
        readonly audioDSPs: AudioDSPList;
    }

    export interface TrackList extends EntityCollection<Track>, OpenDSPXEditable<DSPX.Track[]> {
        /** Returns null for an index outside the list. */
        at(index: number): Track | null;
        /** Creates and inserts a track. The index defaults to length. */
        create(initial?: TrackInit, index?: number): Track;
        /** Imports and inserts one track, including its clips and effects. */
        createFromOpenDSPX(data: DSPX.Track, index?: number): Track;
        /** Destroys the track and its descendants. Returns false if the track is not in this list. */
        delete(track: Track): boolean;
        /** Rotates adjacent ranges without changing entity identity. Requires a transaction. */
        rotate(leftIndex: number, middleIndex: number, rightIndex: number): void;
    }

    export interface Track extends Entity<DSPX.Track> {
        name: string;
        /** The palette ID is an integer. Unavailable IDs use the application's display fallback. */
        colorId: number;
        /** The editor height hint must be positive. */
        height: number;
        /** The linear amplitude gain must be non-negative. */
        gain: number;
        pan: number;
        mute: boolean;
        solo: boolean;
        readonly clips: ClipSequence;
        readonly audioDSPs: AudioDSPList;
        readonly trackList: TrackList | null;
    }

    export type ClipKind = "audio" | "singing";
    export type Clip = AudioClip | SingingClip;

    export interface ClipBase<T extends DSPX.Clip = DSPX.Clip> extends Entity<T> {
        readonly kind: ClipKind;
        name: string;
        /** The linear amplitude gain must be non-negative. */
        gain: number;
        pan: number;
        mute: boolean;
        /** The visible clip begins at this position in project ticks. */
        position: Tick;
        /** The length measures the logical duration of the underlying content. */
        length: Tick;
        /** The content tick at position equals clipStart. */
        clipStart: Tick;
        /** The clipped length measures the visible duration in project ticks. */
        clipLength: Tick;
        /** Equals position - clipStart. May be negative. */
        readonly start: Tick;
        readonly overlapped: boolean;
        readonly previous: Clip | null;
        readonly next: Clip | null;
        readonly clipSequence: ClipSequence | null;
        /** Returns start + localTick. The result may be negative. */
        toProjectTick(localTick: Tick): Tick;
        /** Returns projectTick - start. The result may be negative. */
        toLocalTick(projectTick: Tick): Tick;
    }

    /** Describes audio references without granting file access or decoding the file. */
    export interface AudioPathInfo {
        readonly absoluteDir?: string;
        readonly relativeDir?: string;
        readonly fileName: string;
        readonly formatEntryClassName?: string;
        /** Preserves opaque QVariant data using the same base64 encoding as DSPX audio metadata. */
        readonly serializedUserData?: string;
        readonly digest?: string;
    }

    export interface AudioClip extends ClipBase<DSPX.AudioClip> {
        readonly kind: "audio";
        /** Assignment requires a transaction. Does not copy, read, or validate an audio file. */
        path: AudioPathInfo;
    }

    export interface SingingClip extends ClipBase<DSPX.SingingClip> {
        readonly kind: "singing";
        readonly notes: NoteSequence;
        readonly parameters: ParameterMap;
        readonly sources: Sources | null;
        /** Replaces sources and invalidates the previous configuration and its descendants. */
        setSources(initial: SourcesInit): Sources;
        /** Imports and replaces sources, including singers and dynamic mixing anchors. */
        setSourcesFromOpenDSPX(data: DSPX.Sources): Sources;
        /** Removes sources and invalidates the configuration and its descendants. */
        clearSources(): void;
    }

    /** Clips are ordered by visible position. Overlap is permitted. */
    export interface ClipSequence extends EntityCollection<Clip>, OpenDSPXEditable<DSPX.Clip[]> {
        readonly track: Track;
        readonly audioClipCount: number;
        readonly singingClipCount: number;
        /** Returns clips whose visible intervals intersect the half-open project range. */
        inRange(start: Tick, length: Tick): readonly Clip[];
        /** Creates an audio clip with its required path and time fields. */
        createAudio(initial: AudioClipInit): AudioClip;
        /** Creates a singing clip and its initialized child data. */
        createSinging(initial: SingingClipInit): SingingClip;
        /** Imports and inserts one clip. The DSPX type determines its kind. */
        createFromOpenDSPX(data: DSPX.AudioClip): AudioClip;
        createFromOpenDSPX(data: DSPX.SingingClip): SingingClip;
        createFromOpenDSPX(data: DSPX.Clip): Clip;
        /** Destroys a contained clip and its descendants. Returns false if not contained. */
        delete(clip: Clip): boolean;
        /** Moves a clip from another sequence in the same document without changing its time fields. */
        moveHere(clip: Clip): void;
    }

    export interface NoteSequence extends EntityCollection<Note>, OpenDSPXEditable<DSPX.Note[]> {
        readonly clip: SingingClip;
        /** Returns notes intersecting the half-open clip-local content range. */
        inRange(start: Tick, length: Tick): readonly Note[];
        /** Creates and inserts a note. An omitted lyric uses lyricDefaults. */
        create(initial: NoteInit): Note;
        /** Imports and inserts one note, including pronunciation, phonemes, and vibrato. */
        createFromOpenDSPX(data: DSPX.Note): Note;
        /** Destroys a contained note and its descendants. Returns false if not contained. */
        delete(note: Note): boolean;
        /** Moves a note from another clip in the same document without changing local coordinates. */
        moveHere(note: Note): void;
    }

    export interface Point {
        /** The position is expressed as a ratio of note duration. */
        readonly x: number;
        /** The value multiplies vibrato amplitude or frequency. */
        readonly y: number;
    }

    export interface VibratoPointArray extends Iterable<Point>, OpenDSPXEditable<DSPX.ControlPoint[]> {
        readonly role: "amplitude" | "frequency";
        readonly length: number;
        /** Returns independent point values. */
        toArray(): readonly Point[];
        /** Replaces a bounded index range. Requires a transaction. */
        splice(index: number, deleteCount: number, values?: readonly Point[]): void;
        /** Rotates adjacent ranges. Requires a transaction. */
        rotate(leftIndex: number, middleIndex: number, rightIndex: number): void;
    }

    export interface Note extends Entity<DSPX.Note> {
        /** The pitch shift is an integer from -50 through 50. */
        centShift: number;
        /** The MIDI key number is an integer from 0 through 127. */
        keyNumber: number;
        /** An empty string delegates default-language resolution to the synthesis workflow. */
        language: string;
        length: Tick;
        lyric: string;
        /** The position is a non-negative clip-local content tick. */
        position: Tick;
        readonly originalPronunciation: string;
        /** An empty string uses the generated original pronunciation. */
        editedPronunciation: string;
        readonly overlapped: boolean;
        readonly previous: Note | null;
        readonly next: Note | null;
        readonly noteSequence: NoteSequence | null;
        /** The amplitude is a non-negative integer in cents. */
        vibratoAmplitude: number;
        /** The end is a ratio from 0 through 1. */
        vibratoEnd: number;
        /** The frequency is non-negative and expressed in hertz. */
        vibratoFrequency: number;
        /** The pitch offset is a signed integer in cents. */
        vibratoOffset: number;
        /** The phase must be from 0 through 1. */
        vibratoPhase: number;
        /** The start is a ratio from 0 through 1. */
        vibratoStart: number;
        readonly editedPhonemes: EditablePhonemeSequence;
        readonly originalPhonemes: ReadonlyPhonemeSequence;
        readonly vibratoAmplitudeControlPoints: VibratoPointArray;
        readonly vibratoFrequencyControlPoints: VibratoPointArray;
    }

    /** Original fields are read-only except through explicit whole-object DSPX replacement. */
    export interface Phoneme extends Entity<DSPX.Phoneme> {
        readonly role: "original" | "edited";
        readonly language: string;
        /** The start is an integer millisecond offset from note onset. Negative values are permitted. */
        readonly start: number;
        readonly token: string;
        readonly onset: boolean;
        readonly previous: Phoneme | null;
        readonly next: Phoneme | null;
    }

    export interface EditablePhoneme extends Phoneme {
        readonly role: "edited";
        language: string;
        start: number;
        token: string;
        onset: boolean;
    }

    export interface ReadonlyPhonemeSequence extends EntityCollection<Phoneme>, OpenDSPXEditable<DSPX.Phoneme[]> {
        readonly note: Note;
        readonly role: "original";
        /** Returns phonemes whose starts fall in the millisecond half-open range. */
        inRange(startMilliseconds: number, lengthMilliseconds: number): readonly Phoneme[];
    }

    export interface EditablePhonemeSequence extends EntityCollection<EditablePhoneme>, OpenDSPXEditable<DSPX.Phoneme[]> {
        readonly note: Note;
        readonly role: "edited";
        /** Returns phonemes whose starts fall in the millisecond half-open range. */
        inRange(startMilliseconds: number, lengthMilliseconds: number): readonly EditablePhoneme[];
        /** Creates and inserts an edited phoneme. Starts must be unique within the sequence. */
        create(initial: PhonemeInit): EditablePhoneme;
        /** Imports and inserts one edited phoneme. */
        createFromOpenDSPX(data: DSPX.Phoneme): EditablePhoneme;
        /** Destroys a contained phoneme. Returns false if not contained. */
        delete(phoneme: EditablePhoneme): boolean;
    }

    /** Uses signed DSPX integer values. Null marks an undefined sample. */
    export interface ReadonlyParameterSamples extends Iterable<number | null> {
        readonly parameter: Parameter;
        readonly role: "original" | "transform" | "edited";
        readonly step: 5;
        readonly length: number;
        /** Returns a snapshot. Index i corresponds to content tick i * 5. */
        toArray(): readonly (number | null)[];
        /** Returns undefined outside the array, or null for an undefined sample. */
        at(index: number): number | null | undefined;
    }

    export interface ParameterSamples extends ReadonlyParameterSamples {
        readonly role: "transform" | "edited";
        /** Replaces a bounded index range with signed integers or null. Requires a transaction. */
        splice(index: number, deleteCount: number, values?: readonly (number | null)[]): void;
        /** Rotates adjacent ranges, including undefined samples. Requires a transaction. */
        rotate(leftIndex: number, middleIndex: number, rightIndex: number): void;
    }

    export type AnchorInterpolation = DSPX.AnchorInterpolation;

    export interface AnchorNode extends Entity<DSPX.AnchorNode> {
        interpolationMode: AnchorInterpolation;
        /** The content tick must be non-negative and unique within this sequence. */
        x: Tick;
        /** The parameter value is a signed DSPX integer. */
        y: number;
        readonly previous: AnchorNode | null;
        readonly next: AnchorNode | null;
        readonly sequence: AnchorNodeSequence | null;
    }

    /** Individual nodes use content coordinates. Parameter conversion handles curve-relative offsets. */
    export interface AnchorNodeSequence extends EntityCollection<AnchorNode>, OpenDSPXEditable<DSPX.AnchorNode[]> {
        readonly role: "transform" | "edited";
        readonly parameter: Parameter;
        /** Returns nodes whose x coordinates fall in the half-open content range. */
        inRange(start: Tick, length: Tick): readonly AnchorNode[];
        /** Returns nodes contributing to interpolation, including required neighboring nodes. */
        effectiveInRange(start: Tick, length: Tick): readonly AnchorNode[];
        /** Creates and inserts a node. Its x must be unique. */
        create(initial: AnchorNodeInit): AnchorNode;
        /** Imports a standalone node, treating its x as a content tick. */
        createFromOpenDSPX(data: DSPX.AnchorNode): AnchorNode;
        /** Destroys a contained node. Returns false if not contained. */
        delete(node: AnchorNode): boolean;
        /** Moves a node between layers of the same role in the same document. Requires a unique target x. */
        moveHere(node: AnchorNode): void;
    }

    /**
     * Stores free samples and anchors separately for editable layers.
     * Native export splits free samples at nulls and anchor curves at none interpolation.
     * Native import resamples original anchor curves to free samples. Imported free
     * curves must start on a non-negative multiple of 5. Effective anchor coordinates
     * must be non-negative. Inputs outside the live model's representable domain are rejected.
     * Original anchor coordinates must not exceed 2147483643 during native resampling.
     */
    export interface Parameter extends Entity<DSPX.Param> {
        readonly parameterId: string | null;
        readonly original: ReadonlyParameterSamples;
        readonly freeTransform: ParameterSamples;
        readonly freeEdited: ParameterSamples;
        readonly anchorTransform: AnchorNodeSequence;
        readonly anchorEdited: AnchorNodeSequence;
        readonly parameterMap: ParameterMap | null;
    }

    export interface ParameterMap extends Iterable<readonly [string, Parameter]>, OpenDSPXEditable<DSPX.Params> {
        readonly clip: SingingClip;
        readonly size: number;
        readonly keys: readonly string[];
        /** Returns whether the key exists. */
        has(parameterId: string): boolean;
        /** Returns null for an absent key. */
        get(parameterId: string): Parameter | null;
        /** Returns the existing parameter, or creates an empty parameter within an active transaction. */
        ensure(parameterId: string): Parameter;
        /** Creates a parameter under an unused non-empty key. */
        create(parameterId: string, initial?: ParameterInit): Parameter;
        /** Imports one parameter under an unused non-empty key. */
        createFromOpenDSPX(parameterId: string, data: DSPX.Param): Parameter;
        /** Destroys a parameter and its descendants. Returns false for an absent key. */
        delete(parameterId: string): boolean;
        /** Renames an existing key to an unused non-empty key. Requires a transaction. */
        rename(oldId: string, newId: string): void;
    }

    export interface Sources extends Entity<DSPX.Sources> {
        /** The category identifies the architecture. Unknown identifiers may be preserved in document data. */
        category: string;
        readonly singers: SingerList;
        readonly dynamicMixingAnchors: DynamicMixingAnchorSequence;
        readonly clip: SingingClip | null;
    }

    export type SingerKind = "single" | "mixed";
    export type Singer = SingleSinger | MixedSinger;

    export interface SingerBase<T extends DSPX.Singer = DSPX.Singer> extends Entity<T> {
        readonly kind: SingerKind;
        /** Returns a copy. Assignment replaces the complete value and requires a transaction. */
        extra: JsonValue;
        readonly singerList: SingerList | null;
    }

    export interface SingleSinger extends SingerBase<DSPX.SingleSinger> {
        readonly kind: "single";
        /** The ID identifies a singer within the enclosing Sources.category. */
        id: string;
    }

    export interface MixedSinger extends SingerBase<DSPX.MixedSinger> {
        readonly kind: "mixed";
        /**
         * The array contains weights except for the last child. Each weight is from 0 through 1 and
         * their sum must not exceed 1. A serializable configuration requires
         * singers.length - 1 weights. Assignment copies the array.
         */
        ratio: readonly number[];
        readonly singers: SingerList;
    }

    /**
     * Contains top-level singers or the children of a mixed singer.
     * Structural changes do not automatically rewrite mixed or dynamic weights.
     * The caller must restore consistent weights before exporting or saving.
     */
    export interface SingerList extends EntityCollection<Singer>, OpenDSPXEditable<DSPX.Singer[]> {
        readonly owner: Sources | MixedSinger;
        /** Returns null for an index outside the list. */
        at(index: number): Singer | null;
        /** Creates and inserts a single singer. The index defaults to length. */
        createSingle(initial: SingleSingerInit, index?: number): SingleSinger;
        /** Creates and inserts a mixed singer. The index defaults to length. */
        createMixed(initial?: MixedSingerInit, index?: number): MixedSinger;
        /** Imports and inserts one singer subtree. */
        createFromOpenDSPX(data: DSPX.SingleSinger, index?: number): SingleSinger;
        createFromOpenDSPX(data: DSPX.MixedSinger, index?: number): MixedSinger;
        createFromOpenDSPX(data: DSPX.Singer, index?: number): Singer;
        /** Destroys a contained singer and its descendants. Returns false if not contained. */
        delete(singer: Singer): boolean;
        /** Rotates adjacent ranges without changing entity identity or mixing weights. Requires a transaction. */
        rotate(leftIndex: number, middleIndex: number, rightIndex: number): void;
        /**
         * Moves a singer from another list in the same document. Retains its
         * descendants and extra data. Rejects placement within its own subtree.
         * Does not convert architecture-specific data when moving between sources.
         */
        moveHere(singer: Singer, index?: number): void;
    }

    export interface DynamicMixingAnchor extends Entity<DSPX.DynamicMixingAnchor> {
        /** The content tick must be non-negative and unique within this sequence. */
        position: Tick;
        /** The array contains weights for all top-level singers except the last. Assignment copies it. */
        ratio: readonly number[];
        readonly previous: DynamicMixingAnchor | null;
        readonly next: DynamicMixingAnchor | null;
        readonly sequence: DynamicMixingAnchorSequence | null;
    }

    export interface DynamicMixingAnchorSequence extends EntityCollection<DynamicMixingAnchor>, OpenDSPXEditable<DSPX.DynamicMixingAnchor[]> {
        readonly sources: Sources;
        /** Returns anchors within the half-open content range. */
        inRange(start: Tick, length: Tick): readonly DynamicMixingAnchor[];
        /** Returns anchors contributing to interpolation, including required neighbors. */
        effectiveInRange(start: Tick, length: Tick): readonly DynamicMixingAnchor[];
        /** Creates and inserts an anchor with a unique position. */
        create(initial: DynamicMixingAnchorInit): DynamicMixingAnchor;
        /** Imports and inserts one anchor with a unique position. */
        createFromOpenDSPX(data: DSPX.DynamicMixingAnchor): DynamicMixingAnchor;
        /** Destroys a contained anchor. Returns false if not contained. */
        delete(anchor: DynamicMixingAnchor): boolean;
        /** Moves an anchor from another sequence in the same document. Requires a unique target position. */
        moveHere(anchor: DynamicMixingAnchor): void;
    }

    export interface Tempo extends Entity<DSPX.Tempo> {
        /** The project tick must be non-negative and unique within the tempo sequence. */
        position: Tick;
        /** The tempo is from 10 through 1000 quarter notes per minute. */
        value: number;
        readonly previous: Tempo | null;
        readonly next: Tempo | null;
    }

    export interface TempoSequence extends EntityCollection<Tempo>, OpenDSPXEditable<DSPX.Tempo[]> {
        /** Returns tempo points within the half-open project range. */
        inRange(start: Tick, length: Tick): readonly Tempo[];
        /** Creates and inserts a tempo point with a unique position. */
        create(initial: TempoInit): Tempo;
        /** Imports and inserts one tempo point. */
        createFromOpenDSPX(data: DSPX.Tempo): Tempo;
        /** Destroys a contained tempo point. Returns false if not contained. */
        delete(tempo: Tempo): boolean;
    }

    export interface TimeSignature extends Entity<DSPX.TimeSignature> {
        /** The measure index must be non-negative and unique within the time-signature sequence. */
        index: MeasureIndex;
        /** The numerator must be an integer from 1 through 1118481. */
        numerator: number;
        denominator: DSPX.TimeSignatureDenominator;
        readonly previous: TimeSignature | null;
        readonly next: TimeSignature | null;
    }

    export interface TimeSignatureSequence extends EntityCollection<TimeSignature>, OpenDSPXEditable<DSPX.TimeSignature[]> {
        /** Returns points whose measure indices fall in the half-open measure range. */
        inMeasureRange(startMeasure: MeasureIndex, measureCount: number): readonly TimeSignature[];
        /** Creates and inserts a point with a unique measure index. */
        create(initial: TimeSignatureInit): TimeSignature;
        /** Imports and inserts one time signature. */
        createFromOpenDSPX(data: DSPX.TimeSignature): TimeSignature;
        /** Destroys a contained time signature. Returns false if not contained. */
        delete(timeSignature: TimeSignature): boolean;
    }

    export interface Label extends Entity<DSPX.Label> {
        /** The position is a non-negative project tick. Duplicate positions are permitted. */
        position: Tick;
        text: string;
        readonly previous: Label | null;
        readonly next: Label | null;
    }

    export interface LabelSequence extends EntityCollection<Label>, OpenDSPXEditable<DSPX.Label[]> {
        /** Returns labels within the half-open project range. */
        inRange(start: Tick, length: Tick): readonly Label[];
        /** Creates and inserts a label. */
        create(initial: LabelInit): Label;
        /** Imports and inserts one label. */
        createFromOpenDSPX(data: DSPX.Label): Label;
        /** Destroys a contained label. Returns false if not contained. */
        delete(label: Label): boolean;
    }

    export type AccidentalType = "flat" | "sharp";

    export interface KeySignature extends Entity<DSPX.KeySignature> {
        /** The project tick must be non-negative and unique within the key-signature sequence. */
        position: Tick;
        /** The mode is a 12-bit pitch-class mask relative to the tonic. */
        mode: number;
        /** The tonic is an integer from 0 for C through 11 for B. */
        tonality: number;
        accidentalType: AccidentalType;
        readonly previous: KeySignature | null;
        readonly next: KeySignature | null;
    }

    export interface KeySignatureSequence extends EntityCollection<KeySignature>, OpenDSPXEditable<DSPX.KeySignature[]> {
        /** Returns points within the half-open project range. */
        inRange(start: Tick, length: Tick): readonly KeySignature[];
        /** Creates and inserts a key signature with a unique position. */
        create(initial: KeySignatureInit): KeySignature;
        /** Imports and inserts one DiffScope workspace key signature. */
        createFromOpenDSPX(data: DSPX.KeySignature): KeySignature;
        /** Destroys a contained key signature. Returns false if not contained. */
        delete(keySignature: KeySignature): boolean;
    }

    export interface AudioDSP extends Entity<DSPX.AudioDSP> {
        id: string;
        /** Returns a copy. Assignment replaces the complete effect configuration. */
        data: JsonValue;
        enabled: boolean;
        readonly audioDSPList: AudioDSPList | null;
    }

    export interface AudioDSPList extends EntityCollection<AudioDSP>, OpenDSPXEditable<DSPX.AudioDSP[]> {
        readonly model: DspxModel;
        /** The track is null for the master effect chain. */
        readonly track: Track | null;
        /** Returns null for an index outside the list. */
        at(index: number): AudioDSP | null;
        /** Creates and inserts an effect. The index defaults to length. */
        create(initial: AudioDSPInit, index?: number): AudioDSP;
        /** Imports and inserts one effect configuration. */
        createFromOpenDSPX(data: DSPX.AudioDSP, index?: number): AudioDSP;
        /** Destroys a contained effect. Returns false if not contained. */
        delete(audioDSP: AudioDSP): boolean;
        /** Rotates adjacent ranges without changing entity identity. Requires a transaction. */
        rotate(leftIndex: number, middleIndex: number, rightIndex: number): void;
        /** Moves an effect from another chain in the same document. */
        moveHere(audioDSP: AudioDSP, index?: number): void;
    }

    /**
     * Contains ordinary initialization data, without methods or host references.
     * Creation copies all supplied data. Omitted scalar fields use native model
     * defaults unless documented otherwise. Omitted child collections are empty.
     * Workspace data is copied and explicit model fields take precedence over
     * corresponding DiffScope workspace fields. Creation validates the complete
     * supplied tree before inserting it into the document.
     */
    export interface DspxModelInit {
        readonly projectName?: string;
        readonly projectAuthor?: string;
        readonly globalCentShift?: number;
        readonly multiChannelOutput?: boolean;
        readonly gain?: number;
        readonly pan?: number;
        readonly mute?: boolean;
        readonly loopEnabled?: boolean;
        readonly loopStart?: Tick;
        readonly loopLength?: Tick;
        readonly labels?: readonly LabelInit[];
        readonly keySignatures?: readonly KeySignatureInit[];
        readonly tempos?: readonly TempoInit[];
        readonly timeSignatures?: readonly TimeSignatureInit[];
        readonly tracks?: readonly TrackInit[];
        readonly audioDSPs?: readonly AudioDSPInit[];
        readonly workspace?: DSPX.ModelWorkspace;
    }

    export interface TrackInit {
        readonly name?: string;
        readonly colorId?: number;
        readonly height?: number;
        readonly gain?: number;
        readonly pan?: number;
        readonly mute?: boolean;
        readonly solo?: boolean;
        readonly clips?: readonly ClipInit[];
        readonly audioDSPs?: readonly AudioDSPInit[];
        readonly workspace?: DSPX.TrackWorkspace;
    }

    export interface ClipInitBase {
        readonly name?: string;
        /** The visible position is a non-negative project tick. */
        readonly position: Tick;
        /** The logical content duration must be non-negative. */
        readonly length: Tick;
        /** Defaults to zero. */
        readonly clipStart?: Tick;
        /** The visible duration must be non-negative, in ticks. */
        readonly clipLength: Tick;
        readonly gain?: number;
        readonly pan?: number;
        readonly mute?: boolean;
        readonly workspace?: DSPX.Workspace;
    }

    /** Requires an explicit kind when initializing a heterogeneous clip collection. */
    export type ClipInit = (AudioClipInit & { readonly kind: "audio" }) | (SingingClipInit & { readonly kind: "singing" });

    export interface AudioClipInit extends ClipInitBase {
        /** The kind may be omitted when calling createAudio(). */
        readonly kind?: "audio";
        readonly path: AudioPathInfo;
        readonly workspace?: DSPX.AudioClipWorkspace;
    }

    export interface SingingClipInit extends ClipInitBase {
        /** The kind may be omitted when calling createSinging(). */
        readonly kind?: "singing";
        readonly notes?: readonly NoteInit[];
        readonly parameters?: Readonly<Record<string, ParameterInit>>;
        readonly sources?: SourcesInit | null;
    }

    export interface NoteInit {
        readonly position: Tick;
        readonly length: Tick;
        readonly keyNumber: number;
        readonly centShift?: number;
        /** Uses lyricDefaults for the destination clip after initializing its sources if absent. */
        readonly lyric?: string;
        /** Defaults to an empty string. */
        readonly language?: string;
        readonly editedPronunciation?: string;
        readonly editedPhonemes?: readonly PhonemeInit[];
        /** Initializes a transient cache. Subsequent synthesis may replace it. */
        readonly originalPronunciation?: string;
        readonly originalPhonemes?: readonly PhonemeInit[];
        readonly vibratoAmplitude?: number;
        readonly vibratoEnd?: number;
        readonly vibratoFrequency?: number;
        readonly vibratoOffset?: number;
        readonly vibratoPhase?: number;
        readonly vibratoStart?: number;
        readonly vibratoAmplitudeControlPoints?: readonly Point[];
        readonly vibratoFrequencyControlPoints?: readonly Point[];
        readonly workspace?: DSPX.Workspace;
    }

    export interface PhonemeInit {
        /** The start is an integer millisecond offset from note onset. */
        readonly start: number;
        readonly token: string;
        /** Uses the destination note's language if absent. */
        readonly language?: string;
        /** Defaults to false. */
        readonly onset?: boolean;
    }

    export interface ParameterInit {
        /** Initializes transient samples at five-tick intervals, with null for gaps. */
        readonly original?: readonly (number | null)[];
        readonly freeTransform?: readonly (number | null)[];
        readonly freeEdited?: readonly (number | null)[];
        readonly anchorTransform?: readonly AnchorNodeInit[];
        readonly anchorEdited?: readonly AnchorNodeInit[];
    }

    export interface AnchorNodeInit {
        readonly x: Tick;
        readonly y: number;
        readonly interpolationMode?: AnchorInterpolation;
    }

    export interface SourcesInit {
        readonly category: string;
        readonly singers?: readonly SingerInit[];
        readonly dynamicMixingAnchors?: readonly DynamicMixingAnchorInit[];
    }

    /** Requires an explicit kind when initializing a heterogeneous singer collection. */
    export type SingerInit = (SingleSingerInit & { readonly kind: "single" }) | (MixedSingerInit & { readonly kind: "mixed" });

    export interface SingleSingerInit {
        /** The kind may be omitted when calling createSingle(). */
        readonly kind?: "single";
        readonly id: string;
        /** Uses the registered singer's defaultExtra if absent, or null if unavailable. */
        readonly extra?: JsonValue;
        readonly workspace?: DSPX.Workspace;
    }

    export interface MixedSingerInit {
        /** The kind may be omitted when calling createMixed(). */
        readonly kind?: "mixed";
        /** Defaults to an empty array. Must be adjusted to match initialized children. */
        readonly ratio?: readonly number[];
        readonly singers?: readonly SingerInit[];
        /** Defaults to null. Architecture defaultExtra is not applied automatically. */
        readonly extra?: JsonValue;
        readonly workspace?: DSPX.Workspace;
    }

    export interface DynamicMixingAnchorInit {
        readonly position: Tick;
        /** Contains all top-level singer weights except the last, with the DSPX ratio constraints. */
        readonly ratio: readonly number[];
    }

    export interface TempoInit {
        readonly position: Tick;
        readonly value: number;
    }

    export interface TimeSignatureInit extends TimeSignatureValue {
        readonly index: MeasureIndex;
    }

    export interface LabelInit {
        readonly position: Tick;
        readonly text: string;
    }

    export interface KeySignatureInit {
        readonly position: Tick;
        readonly mode: number;
        readonly tonality: number;
        readonly accidentalType: AccidentalType;
    }

    export interface AudioDSPInit {
        readonly id: string;
        /** Defaults to null. */
        readonly data?: JsonValue;
        /** Defaults to false. */
        readonly enabled?: boolean;
    }
}
