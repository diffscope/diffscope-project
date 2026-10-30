// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

/**
 * Defines plain data for DSPX 1.0.0 and the DiffScope workspace extensions.
 *
 * Field names follow the serialized JSON representation. Objects and arrays are
 * independently editable data, with no connection to a live document. This module
 * exports types only. Type annotations do not validate data obtained from JSON.parse().
 * All numbers must be finite. Integer fields use signed 32-bit integers unless a
 * narrower range is documented. Time uses 480 ticks per quarter note.
 *
 * @see https://dspx.diffscope.org/schema/1.0.0/dspx.schema.json
 */
declare module "diffscope:dspx" {
    export type JsonValue = null | boolean | number | string | JsonValue[] | JsonObject;

    export interface JsonObject {
        [key: string]: JsonValue;
    }

    /** Maps application identifiers to application-specific JSON objects. */
    export type Workspace = Record<string, JsonObject>;

    export type Tick = number;
    export type TimeSignatureDenominator = 1 | 2 | 4 | 8 | 16 | 32 | 64 | 128;
    export type AnchorInterpolation = "none" | "linear" | "hermite";

    export interface Model {
        /** This property may be omitted. OpenDSPX does not retain it in its model. */
        $schema?: string;
        version: "1.0.0";
        content: Content;
    }

    export interface Content {
        global: Global;
        master: Master;
        timeline: Timeline;
        tracks: Track[];
        workspace: ModelWorkspace;
    }

    export interface Global {
        author: string;
        name: string;
        /** The shift is an integer from -50 through 50, in cents. */
        centShift: number;
        editorId: string;
        editorName: string;
    }

    export interface BusControl {
        /** The gain is expressed in decibels. Zero represents unity gain. */
        gain: number;
        /** The pan must be from -1 through 1. */
        pan: number;
        mute: boolean;
    }

    export interface TrackControl extends BusControl {
        solo: boolean;
    }

    export interface Master {
        control: BusControl;
    }

    export interface Timeline {
        labels: Label[];
        tempos: Tempo[];
        timeSignatures: TimeSignature[];
    }

    export interface Label {
        /** The position is a non-negative project tick. Duplicate positions are permitted. */
        pos: Tick;
        text: string;
    }

    export interface Tempo {
        /** The position is a non-negative project tick. */
        pos: Tick;
        /** The tempo is from 10 through 1000 quarter notes per minute. */
        value: number;
    }

    export interface TimeSignature {
        /** The measure index is non-negative and zero-based. */
        index: number;
        /** The numerator must be a positive integer. */
        numerator: number;
        denominator: TimeSignatureDenominator;
    }

    export interface Track {
        name: string;
        control: TrackControl;
        clips: Clip[];
        workspace: TrackWorkspace;
    }

    /** Contains non-negative ticks. The visible project interval is [pos, pos + clipLen). */
    export interface ClipTime {
        /** The visible clip begins at this position in the project timeline. */
        pos: Tick;
        /** The length measures the logical duration of the underlying content. */
        length: Tick;
        /** The content tick at pos equals clipStart. Content tick zero is at pos - clipStart. */
        clipStart: Tick;
        /** The clipped length measures the visible duration. */
        clipLen: Tick;
    }

    export interface ClipBase {
        name: string;
        control: BusControl;
        time: ClipTime;
        workspace: Workspace;
    }

    export type Clip = AudioClip | SingingClip;

    export interface AudioClip extends ClipBase {
        type: "audio";
        /** The path is a string. AudioPathInfo is available through the live model. */
        path: string;
        workspace: AudioClipWorkspace;
    }

    export interface SingingClip extends ClipBase {
        type: "singing";
        notes: Note[];
        params: Params;
        sources: Sources | null;
    }

    export interface Note {
        /** The position is a non-negative tick in the clip's content coordinates. */
        pos: Tick;
        /** The length is a non-negative duration in ticks. */
        length: Tick;
        /** The MIDI key number must be an integer from 0 through 127. */
        keyNum: number;
        /** The shift is an integer from -50 through 50, in cents. */
        centShift: number;
        /** An empty language identifier permits singer-specific language resolution. */
        language: string;
        lyric: string;
        pronunciation: Pronunciation;
        phonemes: Phonemes;
        vibrato: Vibrato;
        workspace: Workspace;
    }

    export interface Pronunciation {
        original: string;
        /** An empty string uses the generated pronunciation. */
        edited: string;
    }

    export interface Phoneme {
        language: string;
        token: string;
        /** The start is an integer millisecond offset from note onset. Negative offsets are permitted. */
        start: number;
        onset: boolean;
    }

    export interface Phonemes {
        original: Phoneme[];
        edited: Phoneme[];
    }

    export interface Vibrato {
        /** The start is a ratio from 0 through 1 of the note duration. */
        start: number;
        /** The end is a ratio from 0 through 1 of the note duration. */
        end: number;
        /** The amplitude is a non-negative integer in cents. */
        amp: number;
        /** The frequency is non-negative and expressed in hertz. */
        freq: number;
        /** The phase must be from 0 through 1. */
        phase: number;
        /** The offset is an integer in cents. */
        offset: number;
        points: VibratoPoints;
    }

    export interface VibratoPoints {
        amp: ControlPoint[];
        freq: ControlPoint[];
    }

    export interface ControlPoint {
        /** The position is expressed as a ratio of the note duration. */
        x: number;
        /** The value multiplies the corresponding vibrato amplitude or frequency. */
        y: number;
    }

    export type Params = Record<string, Param>;

    export interface Param {
        original: ParamCurve[];
        transform: ParamCurve[];
        edited: ParamCurve[];
    }

    export type ParamCurve = ParamCurveFree | ParamCurveAnchor;

    export interface ParamCurveFree {
        type: "free";
        /** The start is a signed tick offset in the clip's content coordinates. */
        start: Tick;
        step: 5;
        /** The samples are integer parameter values. Gaps use separate curves, without null elements. */
        values: number[];
    }

    export interface ParamCurveAnchor {
        type: "anchor";
        /** The start is a signed tick offset in the clip's content coordinates. */
        start: Tick;
        nodes: AnchorNode[];
    }

    export interface AnchorNode {
        /** The position is a non-negative tick relative to the containing curve's start. */
        x: Tick;
        /** The parameter value is a signed integer. */
        y: number;
        /** The interpolation applies from this node to the next node. */
        interp: AnchorInterpolation;
    }

    /**
     * Contains the weights for all singers except the last singer, whose weight
     * is 1 minus their sum. Each weight is from 0 through 1. Their sum must not
     * exceed 1. The array length must equal the singer count minus one.
     */
    export type SourceMixingRatio = number[];

    export interface Sources {
        /** The category identifies the architecture. Registration is not required for stored data. */
        category: string;
        /** Contains at least one singer in a valid serialized configuration. */
        singers: Singer[];
        mix: DynamicMixingAnchor[];
    }

    export interface DynamicMixingAnchor {
        /** The position is a non-negative tick in the clip's content coordinates. */
        pos: Tick;
        /** The weights apply to the top-level Sources.singers list. */
        ratio: SourceMixingRatio;
    }

    export type Singer = SingleSinger | MixedSinger;

    export interface SingerBase {
        extra: JsonValue;
        workspace: Workspace;
    }

    export interface SingleSinger extends SingerBase {
        type: "single";
        /** The ID identifies a singer within Sources.category. */
        id: string;
    }

    export interface MixedSinger extends SingerBase {
        type: "mixed";
        /** Contains at least one singer in a valid serialized configuration. */
        singers: Singer[];
        ratio: SourceMixingRatio;
    }

    /** Defines DiffScope data inside content.workspace.diffscope. Fields may be absent. */
    export type DiffScopeModelWorkspace = JsonObject & {
        loop?: Loop;
        master?: DiffScopeMaster;
        keySignatures?: KeySignature[];
        audioDSP?: AudioDSP[];
    };

    export type ModelWorkspace = Workspace & { diffscope?: DiffScopeModelWorkspace };

    export type Loop = {
        enabled: boolean;
        /** The loop begins at a non-negative project tick. */
        start: Tick;
        /** The loop length is a positive duration in ticks. */
        length: Tick;
    };

    export type DiffScopeMaster = {
        /** Routes each track to a separate stereo pair if true. */
        multiChannelOutput: boolean;
    };

    /** Defines DiffScope data inside track.workspace.diffscope. */
    export type DiffScopeTrackWorkspace = JsonObject & {
        colorId?: number;
        height?: number;
        audioDSP?: AudioDSP[];
    };

    export type TrackWorkspace = Workspace & { diffscope?: DiffScopeTrackWorkspace };

    /** Defines DiffScope data inside audioClip.workspace.diffscope. */
    export type DiffScopeAudioClipWorkspace = JsonObject & {
        audio?: AudioPathMetadata;
    };

    export type AudioClipWorkspace = Workspace & { diffscope?: DiffScopeAudioClipWorkspace };

    export type AudioPathMetadata = {
        relativeDir: string;
        formatEntryClassName: string;
        /** The string contains base64-encoded QVariant data serialized with QDataStream::Qt_5_15. */
        userData: string;
        digest: string;
    };

    /** Defines a DiffScope key signature as a workspace extension. */
    export type KeySignature = {
        /** The position is a non-negative project tick. */
        pos: Tick;
        /** The mode is an integer from 0 through 4095. Bit n represents semitone offset n from the tonic. */
        mode: number;
        /** The tonic is an integer from 0 for C through 11 for B. */
        tonality: number;
        /** Zero selects flats. One selects sharps. */
        accidentalType: 0 | 1;
    };

    /** Defines a DiffScope audio effect configuration as a workspace extension. */
    export type AudioDSP = {
        id: string;
        data: JsonValue;
        enabled: boolean;
    };
}
