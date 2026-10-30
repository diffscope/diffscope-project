// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

(() => {
    "use strict";
    const keys = Object.keys;
    const descriptor = Object.getOwnPropertyDescriptor;
    const create = Object.create;
    const define = Object.defineProperty;
    const isArray = Array.isArray;
    const finite = Number.isFinite;
    const integer = Number.isInteger;
    const Type = TypeError;
    const Range = RangeError;
    const pow = Math.pow;
    const floor = Math.floor;
    const sort = Function.prototype.call.bind(Array.prototype.sort);
    const own = Function.prototype.call.bind(Object.prototype.hasOwnProperty);
    const max = 2147483647;
    const put = (target, name, value) => define(target, name, { value, writable: true, enumerable: true, configurable: true });

    const number = (value, min = -Infinity, upper = Infinity, integral = false) => {
        if (typeof value !== "number") throw new Type("A number is required.");
        if (!finite(value) || value < min || value > upper || (integral && !integer(value))) throw new Range("The number is outside the supported range.");
        return value;
    };
    const int = (value, min = 0, upper = max) => number(value, min, upper, true);
    const string = value => { if (typeof value !== "string") throw new Type("A string is required."); return value; };
    const boolean = value => { if (typeof value !== "boolean") throw new Type("A Boolean is required."); return value; };
    const nonempty = value => { string(value); if (!value.length) throw new Type("A non-empty string is required."); return value; };
    const oneOf = (value, choices) => {
        for (let i = 0; i < choices.length; ++i) if (value === choices[i]) return value;
        throw new Type("The value is not a supported enumeration value.");
    };
    const signature = value => oneOf(value, [1, 2, 4, 8, 16, 32, 64, 128]);
    const object = value => { if (!value || typeof value !== "object" || isArray(value)) throw new Type("An object is required."); return value; };
    const array = value => { if (!isArray(value)) throw new Type("An array is required."); return value; };
    const copy = (value, optional = false, ancestors = [], depth = 0) => {
        if (value === null || typeof value === "string" || typeof value === "boolean") return value;
        if (typeof value === "number") return number(value);
        if (!value || typeof value !== "object") throw new Type("Only JSON data is supported.");
        if (depth > 64) throw new Range("The data nesting depth exceeds the supported limit.");
        for (let i = 0; i < ancestors.length; ++i) if (ancestors[i] === value) throw new Type("Cyclic data is not supported.");
        put(ancestors, depth, value);
        const result = isArray(value) ? [] : create(null);
        const names = keys(value);
        if (isArray(value)) int(value.length);
        for (let i = 0; i < names.length; ++i) {
            const name = names[i];
            const property = descriptor(value, name);
            if (!property || !own(property, "value")) throw new Type("Accessor properties are not supported in document data.");
            if (property.value === undefined && optional && !isArray(value)) continue;
            put(result, name, copy(property.value, optional, ancestors, depth + 1));
        }
        if (isArray(value)) for (let i = 0; i < value.length; ++i) if (!own(result, i)) throw new Type("Sparse arrays are not supported.");
        ancestors.length = depth;
        return result;
    };
    const fields = (value, allowed, required = allowed) => {
        object(value);
        for (let i = 0; i < required.length; ++i) if (!own(value, required[i])) throw new Type("A required document field is missing: " + required[i]);
        const names = keys(value);
        for (let i = 0; i < names.length; ++i) oneOf(names[i], allowed);
    };
    const each = (values, callback) => { array(values); for (let i = 0; i < values.length; ++i) callback(values[i], i); };
    const unique = (values, key) => {
        const seen = create(null);
        each(values, value => { const id = "" + value[key]; if (own(seen, id)) throw new Range("Duplicate sequence positions are not supported."); seen[id] = true; });
    };
    const ratio = (values, count) => {
        let sum = 0;
        each(values, value => { sum += number(value, 0, 1); });
        if (sum > 1 || (count !== undefined && values.length !== (count > 0 ? count - 1 : 0))) throw new Range("The mixing weights do not match the singer configuration.");
    };
    const control = (value, track = false) => {
        const names = track ? ["gain", "pan", "mute", "solo"] : ["gain", "pan", "mute"];
        fields(value, names);
        number(value.gain);
        if (!finite(pow(10, value.gain / 20))) throw new Range("The decibel gain exceeds the live model range.");
        number(value.pan, -1, 1); boolean(value.mute); if (track) boolean(value.solo);
    };
    const point = value => { fields(value, ["x", "y"]); number(value.x); number(value.y); };
    const schema = create(null);
    const validate = (kind, value, incomplete = false) => {
        if (!schema[kind]) throw new Type("Unsupported DSPX type.");
        schema[kind](value, incomplete);
        return value;
    };
    const list = (kind, value, incomplete = false, key) => {
        each(value, item => validate(kind, item, incomplete));
        if (key) unique(value, key);
    };
    const workspace = value => {
        object(value);
        const applications = keys(value);
        for (let i = 0; i < applications.length; ++i) object(value[applications[i]]);
        if (!own(value, "diffscope")) return;
        const scope = value.diffscope;
        if (own(scope, "loop")) {
            object(scope.loop);
            if (own(scope.loop, "enabled")) boolean(scope.loop.enabled);
            if (own(scope.loop, "start")) int(scope.loop.start);
            if (own(scope.loop, "length")) int(scope.loop.length, 1);
            int((own(scope.loop, "start") ? scope.loop.start : 0) + (own(scope.loop, "length") ? scope.loop.length : 1920));
        }
        if (own(scope, "master")) { object(scope.master); if (own(scope.master, "multiChannelOutput")) boolean(scope.master.multiChannelOutput); }
        if (own(scope, "colorId")) int(scope.colorId, -2147483648);
        if (own(scope, "height")) number(scope.height, 5e-324);
        if (own(scope, "keySignatures")) list("KeySignature", scope.keySignatures, false, "pos");
        if (own(scope, "audioDSP")) list("AudioDSP", scope.audioDSP);
        if (own(scope, "audio")) {
            object(scope.audio);
            each(["relativeDir", "formatEntryClassName", "userData", "digest"], name => { if (own(scope.audio, name)) string(scope.audio[name]); });
        }
    };
    schema.Label = value => { fields(value, ["pos", "text"]); int(value.pos); string(value.text); };
    schema.Tempo = value => { fields(value, ["pos", "value"]); int(value.pos); number(value.value, 10, 1000); };
    schema.TimeSignature = value => { fields(value, ["index", "numerator", "denominator"]); int(value.index); int(value.numerator, 1, floor(max / 1920)); signature(value.denominator); };
    schema.KeySignature = value => { fields(value, ["pos", "mode", "tonality", "accidentalType"]); int(value.pos); int(value.mode, 0, 4095); int(value.tonality, 0, 11); oneOf(value.accidentalType, [0, 1]); };
    schema.AudioDSP = value => { fields(value, ["id", "data", "enabled"]); string(value.id); boolean(value.enabled); };
    schema.Phoneme = value => { fields(value, ["start", "token", "language", "onset"]); int(value.start, -2147483648); string(value.token); string(value.language); boolean(value.onset); };
    schema.AnchorNode = value => { fields(value, ["x", "y", "interp"]); int(value.x); int(value.y, -2147483648); oneOf(value.interp, ["none", "linear", "hermite"]); };
    schema.DynamicMixingAnchor = value => { fields(value, ["pos", "ratio"]); int(value.pos); ratio(value.ratio); };
    schema.VibratoPointDataArray = value => each(value, point);
    schema.Parameter = value => {
        fields(value, ["original", "transform", "edited"]);
        each(["original", "transform", "edited"], role => {
            const seen = create(null);
            each(value[role], curve => {
                oneOf(curve.type, ["free", "anchor"]);
                int(curve.start, -2147483648);
                if (curve.type === "free") {
                    fields(curve, ["type", "start", "step", "values"]); oneOf(curve.step, [5]);
                    if (curve.start < 0 || curve.start % 5 !== 0) throw new Range("Free curves must start on a non-negative multiple of five ticks.");
                    each(curve.values, sample => int(sample, -2147483648));
                    int(curve.start + curve.values.length * 5);
                } else {
                    fields(curve, ["type", "start", "nodes"]);
                    let previous = -1;
                    each(curve.nodes, node => {
                        validate("AnchorNode", node);
                        const x = int(curve.start + node.x);
                        if (role === "original") int(x, 0, max - 4);
                        if (node.x <= previous || own(seen, x)) throw new Range("Anchor positions must be strictly increasing and unique within a layer.");
                        previous = node.x; seen[x] = true;
                    });
                }
            });
        });
    };
    schema.ParameterMap = value => { object(value); const names = keys(value); for (let i = 0; i < names.length; ++i) { nonempty(names[i]); validate("Parameter", value[names[i]]); } };
    schema.Note = value => {
        fields(value, ["pos", "length", "keyNum", "centShift", "language", "lyric", "pronunciation", "phonemes", "vibrato", "workspace"]);
        int(value.pos); int(value.length); int(value.pos + value.length); int(value.keyNum, 0, 127); int(value.centShift, -50, 50); string(value.language); string(value.lyric);
        fields(value.pronunciation, ["original", "edited"]); string(value.pronunciation.original); string(value.pronunciation.edited);
        fields(value.phonemes, ["original", "edited"]);
        list("Phoneme", value.phonemes.original, false, "start"); list("Phoneme", value.phonemes.edited, false, "start");
        const vibrato = value.vibrato;
        fields(vibrato, ["amp", "end", "freq", "offset", "phase", "points", "start"]);
        int(vibrato.amp); number(vibrato.end, 0, 1); number(vibrato.freq, 0); int(vibrato.offset, -2147483648); number(vibrato.phase, 0, 1); number(vibrato.start, 0, 1);
        fields(vibrato.points, ["amp", "freq"]); each(vibrato.points.amp, point); each(vibrato.points.freq, point); workspace(value.workspace);
    };
    const singer = (value, incomplete) => {
        oneOf(value.type, ["single", "mixed"]);
        fields(value, value.type === "single" ? ["type", "id", "extra", "workspace"] : ["type", "ratio", "singers", "extra", "workspace"]);
        workspace(value.workspace);
        if (value.type === "single") string(value.id);
        else { list("Singer", value.singers, incomplete); if (!incomplete && !value.singers.length) throw new Range("A mixed singer requires a child singer."); ratio(value.ratio, value.singers.length); }
    };
    schema.Singer = singer;
    schema.SingleSinger = (value, incomplete) => { oneOf(value.type, ["single"]); singer(value, incomplete); };
    schema.MixedSinger = (value, incomplete) => { oneOf(value.type, ["mixed"]); singer(value, incomplete); };
    schema.Sources = (value, incomplete) => {
        fields(value, ["category", "mix", "singers"]); string(value.category); list("Singer", value.singers, incomplete);
        if (!incomplete && !value.singers.length) throw new Range("Sources require a singer.");
        list("DynamicMixingAnchor", value.mix, incomplete, "pos");
        each(value.mix, anchor => ratio(anchor.ratio, value.singers.length));
    };
    const clip = (value, incomplete) => {
        oneOf(value.type, ["audio", "singing"]);
        fields(value, value.type === "audio" ? ["type", "name", "control", "time", "path", "workspace"] : ["type", "name", "control", "time", "notes", "params", "sources", "workspace"]);
        string(value.name); control(value.control); workspace(value.workspace);
        fields(value.time, ["pos", "length", "clipStart", "clipLen"]);
        each(["pos", "length", "clipStart", "clipLen"], name => int(value.time[name]));
        int(value.time.pos + value.time.clipLen); int(value.time.clipStart + value.time.clipLen);
        if (value.type === "audio") string(value.path);
        else { list("Note", value.notes); validate("ParameterMap", value.params); if (value.sources !== null) validate("Sources", value.sources, incomplete); }
    };
    schema.Clip = clip;
    schema.AudioClip = (value, incomplete) => { oneOf(value.type, ["audio"]); clip(value, incomplete); };
    schema.SingingClip = (value, incomplete) => { oneOf(value.type, ["singing"]); clip(value, incomplete); };
    schema.Track = (value, incomplete) => { fields(value, ["name", "control", "clips", "workspace"]); string(value.name); control(value.control, true); list("Clip", value.clips, incomplete); workspace(value.workspace); };
    schema.DspxModel = (value, incomplete) => {
        fields(value, ["version", "content", "$schema"], ["version", "content"]); oneOf(value.version, ["1.0.0"]); if (own(value, "$schema")) string(value.$schema);
        const content = value.content;
        fields(content, ["global", "master", "timeline", "tracks", "workspace"]);
        fields(content.global, ["author", "name", "centShift", "editorId", "editorName"]);
        each(["author", "name", "editorId", "editorName"], name => string(content.global[name]));
        int(content.global.centShift, -50, 50); fields(content.master, ["control"]); control(content.master.control);
        fields(content.timeline, ["labels", "tempos", "timeSignatures"]);
        list("Label", content.timeline.labels); list("Tempo", content.timeline.tempos, false, "pos"); list("TimeSignature", content.timeline.timeSignatures, false, "index");
        list("Track", content.tracks, incomplete); workspace(content.workspace);
    };
    const containers = {
        TrackList: ["Track"], ClipSequence: ["Clip"], NoteSequence: ["Note"], PhonemeSequence: ["Phoneme", "start"], AnchorNodeSequence: ["AnchorNode", "x"],
        SingerList: ["Singer"], DynamicMixingAnchorSequence: ["DynamicMixingAnchor", "pos"], TempoSequence: ["Tempo", "pos"],
        TimeSignatureSequence: ["TimeSignature", "index"], LabelSequence: ["Label"], KeySignatureSequence: ["KeySignature", "pos"], AudioDSPList: ["AudioDSP"],
    };
    for (const name of keys(containers)) {
        const info = containers[name];
        schema[name] = (value, incomplete) => list(info[0], value, incomplete, info[1]);
    }
    return { number, int, string, boolean, nonempty, oneOf, signature, object, array, copy, validate, unique, ratio, each, keys, own, max, pow, floor, sort, put };
})()
