// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

((bridge, V) => {
    "use strict";
    const freeze = Object.freeze;
    const define = Object.defineProperty;
    const create = Object.create;
    const round = Math.round;
    const log10 = Math.log10;
    const Type = TypeError;
    const Range = RangeError;
    const states = new WeakMap();
    const cache = new WeakMap();
    const weakGet = Function.prototype.call.bind(WeakMap.prototype.get);
    const weakSet = Function.prototype.call.bind(WeakMap.prototype.set);
    const own = V.own;
    const keys = V.keys;
    const prototypes = create(null);
    const propertyAliases = { previous: "previousItem", next: "nextItem", sequence: "", clip: "singingClip", length: "size" };
    const scalarSchemas = {
        projectName: V.string, projectAuthor: V.string, globalCentShift: value => V.int(value, -50, 50), multiChannelOutput: V.boolean,
        gain: value => V.number(value, 0), pan: value => V.number(value, -1, 1), mute: V.boolean, solo: V.boolean,
        loopEnabled: V.boolean, loopStart: V.int, loopLength: value => V.int(value, 1), name: V.string, colorId: value => V.int(value, -2147483648),
        height: value => V.number(value, 5e-324), position: V.int, length: V.int, clipStart: V.int, clipLength: V.int,
        centShift: value => V.int(value, -50, 50), keyNumber: value => V.int(value, 0, 127), language: V.string, lyric: V.string,
        editedPronunciation: V.string, vibratoAmplitude: V.int, vibratoEnd: value => V.number(value, 0, 1), vibratoFrequency: value => V.number(value, 0),
        vibratoOffset: value => V.int(value, -2147483648), vibratoPhase: value => V.number(value, 0, 1), vibratoStart: value => V.number(value, 0, 1),
        start: value => V.int(value, -2147483648), token: V.string, onset: V.boolean, x: V.int, y: value => V.int(value, -2147483648),
        interpolationMode: value => V.oneOf(value, ["none", "linear", "hermite"]), category: V.string, id: V.string,
        ratio: value => { V.ratio(value); return value; }, value: value => V.number(value, 10, 1000), index: V.int,
        numerator: value => V.int(value, 1, V.floor(V.max / 1920)), denominator: V.signature, text: V.string,
        mode: value => V.int(value, 0, 4095), tonality: value => V.int(value, 0, 11), accidentalType: value => V.oneOf(value, ["flat", "sharp"]), enabled: V.boolean,
        extra: value => value, data: value => value,
    };
    const enumeration = { interpolationMode: ["none", "linear", "hermite"], accidentalType: ["flat", "sharp"] };
    const entityFields = {
        Track: ["name", "colorId", "height", "gain", "pan", "mute", "solo"],
        AudioClip: ["name", "gain", "pan", "mute", "position", "length", "clipStart", "clipLength", "path"],
        SingingClip: ["name", "gain", "pan", "mute", "position", "length", "clipStart", "clipLength"],
        Note: ["centShift", "keyNumber", "language", "length", "lyric", "position", "editedPronunciation", "vibratoAmplitude", "vibratoEnd", "vibratoFrequency", "vibratoOffset", "vibratoPhase", "vibratoStart"],
        Phoneme: ["language", "start", "token", "onset"], AnchorNode: ["interpolationMode", "x", "y"], Parameter: [],
        Sources: ["category"], SingleSinger: ["id", "extra"], MixedSinger: ["ratio", "extra"], DynamicMixingAnchor: ["position", "ratio"],
        Tempo: ["position", "value"], TimeSignature: ["index", "numerator", "denominator"], Label: ["position", "text"],
        KeySignature: ["position", "mode", "tonality", "accidentalType"], AudioDSP: ["id", "data", "enabled"],
        DspxModel: ["projectName", "projectAuthor", "globalCentShift", "multiChannelOutput", "gain", "pan", "mute", "loopEnabled", "loopStart", "loopLength"],
    };
    const objectFields = {
        DspxModel: ["labels", "keySignatures", "tempos", "timeSignatures", "tracks", "audioDSPs"], Track: ["clips", "audioDSPs", "trackList"],
        AudioClip: ["previous", "next", "clipSequence"], SingingClip: ["previous", "next", "clipSequence", "notes", "parameters", "sources"],
        Note: ["previous", "next", "noteSequence", "editedPhonemes", "originalPhonemes", "vibratoAmplitudeControlPoints", "vibratoFrequencyControlPoints"],
        Phoneme: ["previous", "next"], AnchorNode: ["previous", "next", "sequence"], Parameter: ["original", "freeTransform", "freeEdited", "anchorTransform", "anchorEdited", "parameterMap"],
        Sources: ["singers", "dynamicMixingAnchors", "clip"], SingleSinger: ["singerList"], MixedSinger: ["singerList", "singers"],
        DynamicMixingAnchor: ["previous", "next", "sequence"], Tempo: ["previous", "next"], TimeSignature: ["previous", "next"], Label: ["previous", "next"], KeySignature: ["previous", "next"], AudioDSP: ["audioDSPList"],
    };
    const collections = {
        TrackList: ["Track", "trackList"], ClipSequence: ["Clip", "clipSequence"], NoteSequence: ["Note", "noteSequence"], PhonemeSequence: ["Phoneme", "phonemeSequence", "start"],
        AnchorNodeSequence: ["AnchorNode", "anchorNodeSequence", "x"], SingerList: ["Singer", "singerList"],
        DynamicMixingAnchorSequence: ["DynamicMixingAnchor", "dynamicMixingAnchorSequence", "position"], TempoSequence: ["Tempo", "tempoSequence", "position"],
        TimeSignatureSequence: ["TimeSignature", "timeSignatureSequence", "index"], LabelSequence: ["Label", "labelSequence"], KeySignatureSequence: ["KeySignature", "keySignatureSequence", "position"], AudioDSPList: ["AudioDSP", "audioDSPList"],
    };

    class CoreError extends Error {
        constructor(result) {
            super(result.message);
            this.name = "CoreError";
            this.code = result.code;
            this.operation = result.operation;
            this.project = null;
        }
    }
    freeze(CoreError.prototype);
    const result = value => { if (!value || value.ok !== true) throw new CoreError(value || { code: "unknown", operation: "unknown", message: "The host returned an invalid document result." }); return value.value; };
    const state = (value, requireLive = true) => {
        const entry = weakGet(states, value);
        if (!entry || (requireLive && !bridge.valid(entry.raw, entry.generation))) throw new CoreError({ code: "invalidObject", operation: "access", message: "The document object is no longer available." });
        return entry;
    };
    const active = () => result(bridge.service("active", []));
    const readRaw = (entry, name) => result(bridge.read(entry.raw, name));
    const command = (value, name, args = []) => result(bridge.command(state(value).raw, name, args));
    const requireKind = (value, kind) => { const entry = state(value); if (entry.kind !== kind) throw new Type("The method receiver has an incompatible document type."); return entry; };
    const nativeName = (kind, name) => name === "sequence" ? (kind === "AnchorNode" ? "anchorNodeSequence" : "dynamicMixingAnchorSequence") : name === "clip" && kind === "Sources" ? "singingClip" : propertyAliases[name] && name !== "length" ? propertyAliases[name] : name;
    const get = (value, name, reference = false) => {
        const entry = state(value);
        const raw = readRaw(entry, nativeName(entry.kind, name));
        if (reference) return wrap(raw);
        if (enumeration[name]) return enumeration[name][raw];
        return raw && typeof raw === "object" ? V.copy(raw) : raw;
    };
    const method = (prototype, name, callback) => define(prototype, name, { value: callback });
    const property = (prototype, name, getter, setter) => define(prototype, name, { enumerable: true, get: getter, set: setter });
    const ownerOf = entry => {
        const info = collections[entry.kind];
        if (info) return null;
        const names = ["trackList", "clipSequence", "noteSequence", "phonemeSequence", "parameterMap", "anchorNodeSequence", "dynamicMixingAnchorSequence", "tempoSequence", "timeSignatureSequence", "labelSequence", "keySignatureSequence", "singerList", "audioDSPList"];
        for (let i = 0; i < names.length; ++i) {
            const raw = readRaw(entry, names[i]);
            if (raw) return { raw, kind: bridge.kind(raw) };
        }
        return null;
    };
    const rawItems = entry => result(bridge.items(entry.raw));
    const uniqueness = (container, name, number, excluded) => {
        const values = rawItems(container);
        for (let i = 0; i < values.length; ++i) if (values[i] !== excluded && result(bridge.read(values[i], name)) === number) throw new Range("The target sequence already contains this position.");
    };
    const checkIntervals = (entry, name, value) => {
        const current = key => name === key ? value : readRaw(entry, key);
        if (entry.kind === "Note" && (name === "position" || name === "length")) V.int(current("position") + current("length"));
        if ((entry.kind === "AudioClip" || entry.kind === "SingingClip") && (name === "position" || name === "clipLength" || name === "clipStart")) { V.int(current("position") + current("clipLength")); V.int(current("clipStart") + current("clipLength")); }
        if (entry.kind === "DspxModel" && (name === "loopStart" || name === "loopLength")) V.int(current("loopStart") + current("loopLength"));
        const owner = ownerOf(entry);
        if (owner && collections[owner.kind] && collections[owner.kind][2] === name) uniqueness(owner, name, value, entry.raw);
    };
    const mutation = (entry, callback) => {
        active();
        if (!bridge.valid(entry.raw, entry.generation)) throw new CoreError({ code: "invalidObject", operation: "modify", message: "The document object is no longer available." });
        let completed = false;
        try { const value = callback(); result(bridge.complete(entry.raw)); completed = true; return value; }
        finally { if (!completed) bridge.complete(entry.raw); }
    };
    const set = (value, name, input) => {
        const entry = state(value);
        active();
        let data = name === "extra" || name === "data" || (typeof input === "object" && input !== null) ? V.copy(input) : input;
        if (entry.kind === "Phoneme" && readRaw(entry, "role") === 0) throw new Type("Original phoneme properties are read-only.");
        if (name === "path") {
            V.object(data); V.string(data.fileName);
            for (let i = 0, names = ["absoluteDir", "relativeDir", "formatEntryClassName", "serializedUserData", "digest"]; i < names.length; ++i) { const key = names[i]; if (!own(data, key)) data[key] = ""; V.string(data[key]); }
        } else { scalarSchemas[name](data); checkIntervals(entry, name, data); }
        if (enumeration[name]) { const names = enumeration[name]; for (let i = 0; i < names.length; ++i) if (names[i] === data) { data = i; break; } }
        mutation(entry, () => result(bridge.write(entry.raw, name, data)));
    };
    const toDSPX = value => V.copy(result(bridge.snapshot(state(value).raw)));
    const fromDSPX = (value, input) => {
        const entry = state(value); active();
        const data = V.validate(entry.kind, V.copy(input));
        state(value);
        if (entry.kind === "AnchorNode" || entry.kind === "Tempo" || entry.kind === "TimeSignature" || entry.kind === "KeySignature" || entry.kind === "DynamicMixingAnchor" || entry.kind === "Phoneme") {
            const owner = ownerOf(entry);
            const key = entry.kind === "AnchorNode" ? "x" : entry.kind === "TimeSignature" ? "index" : entry.kind === "Phoneme" ? "start" : "position";
            if (owner) uniqueness(owner, key, data[key === "position" ? "pos" : key], entry.raw);
        }
        mutation(entry, () => result(bridge.replace(entry.raw, data, false)));
    };
    const wrap = raw => {
        if (raw === null || raw === undefined) return null;
        const kind = bridge.kind(raw);
        const generation = bridge.generation(raw);
        if (!kind || !generation || !bridge.valid(raw, generation)) return undefined;
        const existing = weakGet(cache, raw);
        if (existing && weakGet(states, existing).generation === generation) return existing;
        const value = create(prototypes[kind]);
        weakSet(states, value, { raw, kind, generation, handle: bridge.identity(raw) }); weakSet(cache, raw, value);
        return freeze(value);
    };
    const unwrap = value => {
        const entry = weakGet(states, value);
        return entry && bridge.valid(entry.raw, entry.generation) ? entry.raw : undefined;
    };

    const entityPrototype = create(null);
    property(entityPrototype, "handle", function () { return state(this, false).handle; });
    property(entityPrototype, "valid", function () { const entry = state(this, false); return bridge.valid(entry.raw, entry.generation); });
    property(entityPrototype, "model", function () { return wrap(readRaw(state(this), "model")); });
    method(entityPrototype, "toOpenDSPX", function () { return toDSPX(this); });
    method(entityPrototype, "fromOpenDSPX", function (data) { fromDSPX(this, data); });
    for (let i = 0, names = keys(entityFields); i < names.length; ++i) {
        const name = names[i];
        const prototype = prototypes[name] = create(entityPrototype);
        const properties = entityFields[name];
        for (let j = 0; j < properties.length; ++j) { const key = properties[j]; property(prototype, key, function () { return get(this, key); }, function (value) { set(this, key, value); }); }
        const references = objectFields[name] || [];
        for (let j = 0; j < references.length; ++j) { const key = references[j]; property(prototype, key, function () { return get(this, key, true); }); }
    }
    for (let i = 0, names = ["AudioClip", "SingingClip", "Note"]; i < names.length; ++i) {
        const name = names[i]; property(prototypes[name], "overlapped", function () { return get(this, "overlapped"); });
    }
    for (let i = 0, names = ["AudioClip", "SingingClip"]; i < names.length; ++i) {
        const name = names[i];
        property(prototypes[name], "kind", () => name === "AudioClip" ? "audio" : "singing");
        property(prototypes[name], "start", function () { return get(this, "start"); });
        method(prototypes[name], "toProjectTick", function (tick) { V.int(tick, -2147483648); return V.int(this.start + tick, -2147483648); });
        method(prototypes[name], "toLocalTick", function (tick) { V.int(tick, -2147483648); return V.int(tick - this.start, -2147483648); });
    }
    property(prototypes.SingleSinger, "kind", () => "single"); property(prototypes.MixedSinger, "kind", () => "mixed");
    property(prototypes.Note, "originalPronunciation", function () { return get(this, "originalPronunciation"); });
    property(prototypes.Parameter, "parameterId", function () { return get(this, "parameterId"); });
    property(prototypes.Phoneme, "role", function () { return get(this, "role") === 0 ? "original" : "edited"; });

    const mapped = (items, callback) => { const output = []; V.each(items, (item, index) => { V.put(output, index, callback(item, index)); }); return output; };
    const defaulted = (input, name, fallback) => own(input, name) ? input[name] : fallback;
    const bus = input => {
        const gain = V.number(defaulted(input, "gain", 1), 0);
        return { gain: gain === 0 ? -1e308 : 20 * log10(gain), pan: defaulted(input, "pan", 0), mute: defaulted(input, "mute", false) };
    };
    const extension = workspace => {
        V.object(workspace);
        if (!own(workspace, "diffscope")) workspace.diffscope = create(null);
        V.object(workspace.diffscope);
        return workspace.diffscope;
    };
    const initNames = {
        DspxModel: ["projectName", "projectAuthor", "globalCentShift", "multiChannelOutput", "gain", "pan", "mute", "loopEnabled", "loopStart", "loopLength", "labels", "keySignatures", "tempos", "timeSignatures", "tracks", "audioDSPs", "workspace"],
        Track: ["name", "colorId", "height", "gain", "pan", "mute", "solo", "clips", "audioDSPs", "workspace"],
        AudioClip: ["kind", "name", "position", "length", "clipStart", "clipLength", "gain", "pan", "mute", "workspace", "path"],
        SingingClip: ["kind", "name", "position", "length", "clipStart", "clipLength", "gain", "pan", "mute", "workspace", "notes", "parameters", "sources"],
        Note: ["position", "length", "keyNumber", "centShift", "lyric", "language", "editedPronunciation", "editedPhonemes", "originalPronunciation", "originalPhonemes", "vibratoAmplitude", "vibratoEnd", "vibratoFrequency", "vibratoOffset", "vibratoPhase", "vibratoStart", "vibratoAmplitudeControlPoints", "vibratoFrequencyControlPoints", "workspace"],
        Phoneme: ["start", "token", "language", "onset"], Parameter: ["original", "freeTransform", "freeEdited", "anchorTransform", "anchorEdited"],
        AnchorNode: ["x", "y", "interpolationMode"], Sources: ["category", "singers", "dynamicMixingAnchors"], SingleSinger: ["kind", "id", "extra", "workspace"], MixedSinger: ["kind", "singers", "ratio", "extra", "workspace"],
        DynamicMixingAnchor: ["position", "ratio"], Tempo: ["position", "value"], TimeSignature: ["index", "numerator", "denominator"], Label: ["position", "text"], KeySignature: ["position", "mode", "tonality", "accidentalType"], AudioDSP: ["id", "data", "enabled"],
    };
    const freeCurves = samples => {
        const curves = [];
        let current = null;
        V.each(samples, (sample, index) => {
            if (sample === null) { current = null; return; }
            V.int(sample, -2147483648);
            if (!current) { current = { type: "free", start: index * 5, step: 5, values: [] }; curves[curves.length] = current; }
            current.values[current.values.length] = sample;
        });
        return curves;
    };
    const anchorCurves = nodes => {
        const points = mapped(nodes, node => initData("AnchorNode", node));
        V.sort(points, (a, b) => a.x - b.x);
        V.unique(points, "x");
        const curves = [];
        let current = null;
        V.each(points, point => {
            if (!current) { current = { type: "anchor", start: point.x, nodes: [] }; curves[curves.length] = current; }
            current.nodes[current.nodes.length] = { x: point.x - current.start, y: point.y, interp: point.interp };
            if (point.interp === "none") current = null;
        });
        V.each(curves, curve => { curve.nodes[curve.nodes.length - 1].interp = "none"; });
        return curves;
    };
    const append = (left, right) => { for (let i = 0; i < right.length; ++i) V.put(left, left.length, right[i]); return left; };
    const lyricForSources = sources => {
        const fallback = result(bridge.service("lyric", [null]));
        if (!sources) return fallback;
        let first;
        const visit = singers => {
            for (let i = 0; i < singers.length && first === undefined; ++i) {
                const singer = singers[i];
                if (singer.type === "single") { if (singer.id.length) first = singer.id; }
                else visit(singer.singers);
            }
        };
        visit(sources.singers);
        if (first === undefined) return fallback;
        const singer = result(bridge.service("singer", [sources.category, first]));
        if (!singer || !singer.languages.length) return fallback;
        const language = singer.defaultLanguage || singer.languages[0].id;
        for (let i = 0; i < singer.languages.length; ++i) if (singer.languages[i].id === language) return singer.languages[i].defaultLyric || fallback;
        return fallback;
    };
    const initData = (kind, input = {}, context = {}) => {
        V.object(input);
        const allowed = initNames[kind];
        const names = keys(input);
        for (let i = 0; i < names.length; ++i) V.oneOf(names[i], allowed);
        const w = own(input, "workspace") ? input.workspace : create(null);
        const d = (name, fallback) => defaulted(input, name, fallback);
        const childList = (name, type, ctx = context) => mapped(d(name, []), value => initData(type, value, ctx));
        const singerList = (name, ctx) => mapped(d(name, []), value => initData(value.kind === "single" ? "SingleSinger" : value.kind === "mixed" ? "MixedSinger" : V.oneOf(value.kind, ["single", "mixed"]), value, ctx));
        let value;
        if (kind === "DspxModel") {
            const scope = extension(w);
            const loop = own(scope, "loop") ? scope.loop : {};
            scope.loop = { enabled: d("loopEnabled", defaulted(loop, "enabled", false)), start: d("loopStart", defaulted(loop, "start", 0)), length: d("loopLength", defaulted(loop, "length", 1920)) };
            const master = own(scope, "master") ? scope.master : {};
            scope.master = { multiChannelOutput: d("multiChannelOutput", defaulted(master, "multiChannelOutput", false)) };
            if (own(input, "keySignatures")) scope.keySignatures = childList("keySignatures", "KeySignature");
            if (own(input, "audioDSPs")) scope.audioDSP = childList("audioDSPs", "AudioDSP");
            value = { version: "1.0.0", content: { global: { name: d("projectName", ""), author: d("projectAuthor", ""), centShift: d("globalCentShift", 0), editorId: "org.diffscope.diffscope", editorName: "DiffScope" }, master: { control: bus(input) }, timeline: { labels: childList("labels", "Label"), tempos: childList("tempos", "Tempo"), timeSignatures: childList("timeSignatures", "TimeSignature") }, tracks: childList("tracks", "Track"), workspace: w } };
        } else if (kind === "Track") {
            const scope = extension(w);
            scope.colorId = d("colorId", defaulted(scope, "colorId", 0)); scope.height = d("height", defaulted(scope, "height", 80));
            if (own(input, "audioDSPs")) scope.audioDSP = childList("audioDSPs", "AudioDSP");
            const control = bus(input); control.solo = d("solo", false);
            const clips = mapped(d("clips", []), clip => initData(clip.kind === "audio" ? "AudioClip" : clip.kind === "singing" ? "SingingClip" : V.oneOf(clip.kind, ["audio", "singing"]), clip, context));
            value = { name: d("name", ""), control, clips, workspace: w };
        } else if (kind === "AudioClip" || kind === "SingingClip") {
            if (own(input, "kind")) V.oneOf(input.kind, [kind === "AudioClip" ? "audio" : "singing"]);
            value = { type: kind === "AudioClip" ? "audio" : "singing", name: d("name", ""), control: bus(input), time: { pos: input.position, length: input.length, clipStart: d("clipStart", 0), clipLen: input.clipLength }, workspace: w };
            if (kind === "AudioClip") {
                const path = V.object(input.path);
                V.string(path.fileName);
                const directory = defaulted(path, "absoluteDir", ""); V.string(directory);
                value.path = directory.length ? directory + "/" + path.fileName : path.fileName;
                const audio = { relativeDir: defaulted(path, "relativeDir", ""), formatEntryClassName: defaulted(path, "formatEntryClassName", ""), userData: defaulted(path, "serializedUserData", ""), digest: defaulted(path, "digest", "") };
                extension(w).audio = audio;
            } else {
                value.sources = !own(input, "sources") || input.sources === null ? null : initData("Sources", input.sources);
                const lyric = lyricForSources(value.sources);
                value.notes = childList("notes", "Note", { lyric });
                value.params = create(null);
                const parameters = d("parameters", {}); V.object(parameters);
                const ids = keys(parameters);
                for (let i = 0; i < ids.length; ++i) value.params[ids[i]] = initData("Parameter", parameters[ids[i]]);
            }
        } else if (kind === "Note") {
            value = { pos: input.position, length: input.length, keyNum: input.keyNumber, centShift: d("centShift", 0), language: d("language", ""), lyric: d("lyric", context.lyric === undefined ? result(bridge.service("lyric", [null])) : context.lyric), pronunciation: { original: d("originalPronunciation", ""), edited: d("editedPronunciation", "") }, phonemes: { original: childList("originalPhonemes", "Phoneme", { language: d("language", "") }), edited: childList("editedPhonemes", "Phoneme", { language: d("language", "") }) }, vibrato: { amp: d("vibratoAmplitude", 0), end: d("vibratoEnd", 0), freq: d("vibratoFrequency", 0), offset: d("vibratoOffset", 0), phase: d("vibratoPhase", 0), start: d("vibratoStart", 0), points: { amp: d("vibratoAmplitudeControlPoints", []), freq: d("vibratoFrequencyControlPoints", []) } }, workspace: w };
        } else if (kind === "Phoneme") value = { start: input.start, token: input.token, language: d("language", context.language === undefined ? "" : context.language), onset: d("onset", false) };
        else if (kind === "Parameter") value = { original: freeCurves(d("original", [])), transform: append(freeCurves(d("freeTransform", [])), anchorCurves(d("anchorTransform", []))), edited: append(freeCurves(d("freeEdited", [])), anchorCurves(d("anchorEdited", []))) };
        else if (kind === "AnchorNode") value = { x: input.x, y: input.y, interp: d("interpolationMode", "none") };
        else if (kind === "Sources") value = { category: input.category, singers: singerList("singers", { category: input.category }), mix: childList("dynamicMixingAnchors", "DynamicMixingAnchor") };
        else if (kind === "SingleSinger" || kind === "MixedSinger") {
            if (own(input, "kind")) V.oneOf(input.kind, [kind === "SingleSinger" ? "single" : "mixed"]);
            let extra = d("extra", null);
            if (kind === "SingleSinger" && !own(input, "extra") && context.category !== undefined) { const singer = result(bridge.service("singer", [context.category, input.id])); if (singer) extra = singer.defaultExtra; }
            value = kind === "SingleSinger" ? { type: "single", id: input.id, extra, workspace: w } : { type: "mixed", singers: singerList("singers", context), ratio: d("ratio", []), extra, workspace: w };
        } else if (kind === "DynamicMixingAnchor") value = { pos: input.position, ratio: input.ratio };
        else if (kind === "Tempo") value = { pos: input.position, value: input.value };
        else if (kind === "TimeSignature") value = { index: input.index, numerator: input.numerator, denominator: input.denominator };
        else if (kind === "Label") value = { pos: input.position, text: input.text };
        else if (kind === "KeySignature") value = { pos: input.position, mode: input.mode, tonality: input.tonality, accidentalType: V.oneOf(input.accidentalType, ["flat", "sharp"]) === "flat" ? 0 : 1 };
        else if (kind === "AudioDSP") value = { id: input.id, data: d("data", null), enabled: d("enabled", false) };
        return V.validate(kind, value, true);
    };
    const importContext = entry => {
        if (entry.kind === "NoteSequence") return { lyric: result(bridge.service("lyric", [readRaw(entry, "singingClip")])) };
        if (entry.kind === "PhonemeSequence") return { language: result(bridge.read(readRaw(entry, "note"), "language")) };
        if (entry.kind === "SingerList") {
            let source = readRaw(entry, "sources");
            let parent = readRaw(entry, "mixedSinger");
            while (!source && parent) { const list = result(bridge.read(parent, "singerList")); source = result(bridge.read(list, "sources")); parent = result(bridge.read(list, "mixedSinger")); }
            return { category: source ? result(bridge.read(source, "category")) : undefined };
        }
        return {};
    };
    const createItem = (container, kind, input, index, key = "", imported = false) => {
        const entry = state(container); active();
        if (entry.kind === "PhonemeSequence" && readRaw(entry, "role") === 0) throw new Type("Original phoneme collections support whole-collection replacement only.");
        const data = imported ? V.validate(kind, V.copy(input)) : initData(kind, V.copy(input === undefined ? {} : input, true), importContext(entry));
        state(container);
        const count = rawItems(entry).length;
        const target = index === undefined ? count : V.int(index, 0, count);
        const info = collections[entry.kind];
        if (info && info[2]) uniqueness(entry, info[2], data[info[2] === "position" ? "pos" : info[2]]);
        if (entry.kind === "ParameterMap") { V.nonempty(key); if (parameterRaw(entry, key)) throw new Range("The parameter key already exists."); }
        return mutation(entry, () => {
            const original = entry.kind === "PhonemeSequence" && readRaw(entry, "role") === 0;
            const raw = result(bridge.allocate(entry.raw, kind, original));
            result(bridge.replace(raw, data, !imported));
            result(bridge.attach(entry.raw, raw, target, key));
            return wrap(raw);
        });
    };
    const parameterRaw = (entry, id) => {
        const ids = readRaw(entry, "keys"); const values = rawItems(entry);
        for (let i = 0; i < ids.length; ++i) if (ids[i] === id) return values[i];
        return null;
    };

    const snapshotItems = value => mapped(rawItems(state(value)), wrap);
    const rotateItems = kind => function (leftIndex, middleIndex, rightIndex) {
        const entry = requireKind(this, kind);
        if (kind === "FreeValueDataArray" && readRaw(entry, "role") === 0) throw new Type("Original samples are read-only.");
        const length = readRaw(entry, "size");
        V.int(leftIndex, 0, length); V.int(middleIndex, leftIndex, length); V.int(rightIndex, middleIndex, length);
        mutation(entry, () => result(bridge.rotate(entry.raw, leftIndex, middleIndex, rightIndex)));
    };
    const collectionPrototype = create(null);
    property(collectionPrototype, "length", function () { return rawItems(state(this)).length; });
    property(collectionPrototype, "first", function () { const values = rawItems(state(this)); return values.length ? wrap(values[0]) : null; });
    property(collectionPrototype, "last", function () { const values = rawItems(state(this)); return values.length ? wrap(values[values.length - 1]) : null; });
    method(collectionPrototype, "toArray", function () { return freeze(snapshotItems(this)); });
    method(collectionPrototype, Symbol.iterator, function () {
        const values = snapshotItems(this); let index = 0;
        return { next: () => index < values.length ? { value: values[index++], done: false } : { done: true } };
    });
    method(collectionPrototype, "has", function (item) {
        const target = state(this); const entry = weakGet(states, item);
        if (!entry || !bridge.valid(entry.raw, entry.generation)) return false;
        const values = rawItems(target); for (let i = 0; i < values.length; ++i) if (values[i] === entry.raw) return true;
        return false;
    });
    method(collectionPrototype, "delete", function (item) {
        const entry = state(this);
        if (entry.kind === "PhonemeSequence" && readRaw(entry, "role") === 0) throw new Type("Original phoneme collections support whole-collection replacement only.");
        if (!this.has(item)) return false;
        return mutation(entry, () => { result(bridge.destroy(state(item).raw)); return true; });
    });
    method(collectionPrototype, "toOpenDSPX", function () { return toDSPX(this); });
    method(collectionPrototype, "fromOpenDSPX", function (data) { fromDSPX(this, data); });
    for (let i = 0, names = keys(collections); i < names.length; ++i) {
        const kind = names[i]; const info = collections[kind];
        const prototype = prototypes[kind] = create(collectionPrototype);
        if (kind !== "ClipSequence" && kind !== "SingerList") {
            method(prototype, "create", function (input, index) { requireKind(this, kind); return createItem(this, info[0], input, index); });
        }
        method(prototype, "createFromOpenDSPX", function (data, index) {
            requireKind(this, kind);
            const copied = V.copy(data);
            const type = kind === "ClipSequence" ? copied.type === "audio" ? "AudioClip" : "SingingClip" : kind === "SingerList" ? copied.type === "single" ? "SingleSinger" : "MixedSinger" : info[0];
            return createItem(this, type, copied, index, "", true);
        });
        method(prototype, "at", function (index) { V.int(index); const items = rawItems(state(this)); return index < items.length ? wrap(items[index]) : null; });
        const range = function (start, length, effective = false) {
            V.int(start, kind === "PhonemeSequence" ? -2147483648 : 0); V.int(length); V.int(start + length, kind === "PhonemeSequence" ? -2147483648 : 0);
            const entry = state(this); const items = rawItems(entry); const resultItems = [];
            if (!length) return freeze(resultItems);
            let previous = null; let following = null;
            for (let j = 0; j < items.length; ++j) {
                const item = items[j]; const key = kind === "AnchorNodeSequence" ? "x" : kind === "PhonemeSequence" ? "start" : kind === "TimeSignatureSequence" ? "index" : "position";
                const position = result(bridge.read(item, key));
                const interval = kind === "NoteSequence" || kind === "ClipSequence";
                const end = interval ? position + result(bridge.read(item, kind === "ClipSequence" ? "clipLength" : "length")) : position;
                if ((interval ? end > start : position >= start) && position < start + length) resultItems[resultItems.length] = wrap(item);
                if (position < start) previous = item;
                else if (position >= start + length && following === null) following = item;
            }
            if (effective) { if (previous) { const withPrevious = [wrap(previous)]; append(withPrevious, resultItems); resultItems.length = 0; append(resultItems, withPrevious); } if (following) resultItems[resultItems.length] = wrap(following); }
            return freeze(resultItems);
        };
        method(prototype, kind === "TimeSignatureSequence" ? "inMeasureRange" : "inRange", range);
        if (kind === "AnchorNodeSequence" || kind === "DynamicMixingAnchorSequence") method(prototype, "effectiveInRange", function (start, length) { return range.call(this, start, length, true); });
        if (kind === "ClipSequence" || kind === "NoteSequence" || kind === "AnchorNodeSequence" || kind === "DynamicMixingAnchorSequence" || kind === "SingerList" || kind === "AudioDSPList") {
            method(prototype, "moveHere", function (item, index) {
                const target = requireKind(this, kind); const source = state(item); active();
                if (!bridge.sameDocument(target.raw, source.raw)) throw new Type("Entities must belong to the same document.");
                const sourceOwner = ownerOf(source);
                if (!sourceOwner || sourceOwner.kind !== target.kind) throw new Type("The entity has an incompatible owner.");
                if (sourceOwner.raw === target.raw) throw new Type("A different source container is required.");
                const length = rawItems(target).length;
                const targetIndex = index === undefined ? length : V.int(index, 0, length);
                if (info[2]) uniqueness(target, info[2], readRaw(source, info[2]), source.raw);
                if (kind === "AnchorNodeSequence" && readRaw(target, "role") !== readRaw(sourceOwner, "role")) throw new Type("Anchor layer roles must match.");
                if (kind === "SingerList") {
                    let ancestor = readRaw(target, "mixedSinger");
                    while (ancestor) {
                        if (ancestor === source.raw) throw new Type("A singer cannot be moved into its own descendants.");
                        const list = result(bridge.read(ancestor, "singerList")); ancestor = list ? result(bridge.read(list, "mixedSinger")) : null;
                    }
                }
                mutation(target, () => result(bridge.transfer(target.raw, source.raw, targetIndex)));
            });
        }
        if (kind === "TrackList" || kind === "SingerList" || kind === "AudioDSPList") {
            method(prototype, "rotate", rotateItems(kind));
        }
    }
    method(prototypes.ClipSequence, "createAudio", function (input) { return createItem(this, "AudioClip", input); });
    method(prototypes.ClipSequence, "createSinging", function (input) { return createItem(this, "SingingClip", input); });
    method(prototypes.SingerList, "createSingle", function (input, index) { return createItem(this, "SingleSinger", input, index); });
    method(prototypes.SingerList, "createMixed", function (input, index) { return createItem(this, "MixedSinger", input, index); });
    property(prototypes.SingerList, "owner", function () { const entry = state(this); return wrap(readRaw(entry, "sources") || readRaw(entry, "mixedSinger")); });
    const collectionOwners = { ClipSequence: ["track", "track"], NoteSequence: ["clip", "singingClip"], PhonemeSequence: ["note", "note"], AnchorNodeSequence: ["parameter", "parameter"], DynamicMixingAnchorSequence: ["sources", "sources"], AudioDSPList: ["track", "track"] };
    for (let i = 0, names = keys(collectionOwners); i < names.length; ++i) {
        const info = collectionOwners[names[i]]; property(prototypes[names[i]], info[0], function () { return wrap(readRaw(state(this), info[1])); });
    }
    property(prototypes.AudioDSPList, "model", function () { return wrap(result(bridge.model(state(this).raw))); });
    property(prototypes.ClipSequence, "audioClipCount", function () { return readRaw(state(this), "audioClipCount"); });
    property(prototypes.ClipSequence, "singingClipCount", function () { return readRaw(state(this), "singingClipCount"); });
    property(prototypes.PhonemeSequence, "role", function () { return readRaw(state(this), "role") === 0 ? "original" : "edited"; });
    property(prototypes.AnchorNodeSequence, "role", function () { return readRaw(state(this), "role") === 0 ? "transform" : "edited"; });
    method(prototypes.SingingClip, "setSources", function (input) { return createItem(this, "Sources", input); });
    method(prototypes.SingingClip, "setSourcesFromOpenDSPX", function (input) { return createItem(this, "Sources", input, undefined, "", true); });
    method(prototypes.SingingClip, "clearSources", function () { const entry = state(this); mutation(entry, () => result(bridge.write(entry.raw, "sources", null))); });

    const mapPrototype = prototypes.ParameterMap = create(null);
    property(mapPrototype, "clip", function () { return wrap(readRaw(state(this), "singingClip")); });
    property(mapPrototype, "size", function () { return readRaw(state(this), "size"); });
    property(mapPrototype, "keys", function () { return freeze(mapped(readRaw(state(this), "keys"), value => value)); });
    method(mapPrototype, "get", function (id) { V.string(id); return wrap(parameterRaw(state(this), id)); });
    method(mapPrototype, "has", function (id) { return this.get(id) !== null; });
    method(mapPrototype, "ensure", function (id) { V.nonempty(id); return this.get(id) || createItem(this, "Parameter", {}, undefined, id); });
    method(mapPrototype, "create", function (id, input) { return createItem(this, "Parameter", input, undefined, V.nonempty(id)); });
    method(mapPrototype, "createFromOpenDSPX", function (id, input) { return createItem(this, "Parameter", input, undefined, V.nonempty(id), true); });
    method(mapPrototype, "delete", function (id) { V.string(id); const item = this.get(id); if (!item) return false; const entry = state(this); return mutation(entry, () => { result(bridge.destroy(state(item).raw)); return true; }); });
    method(mapPrototype, "rename", function (oldId, newId) { V.nonempty(oldId); V.nonempty(newId); if (!this.has(oldId) || this.has(newId)) throw new Range("The parameter keys are not available for renaming."); command(this, "renameParameter", [oldId, newId]); });
    method(mapPrototype, "toOpenDSPX", function () { return toDSPX(this); });
    method(mapPrototype, "fromOpenDSPX", function (input) { fromDSPX(this, input); });
    method(mapPrototype, Symbol.iterator, function () { const ids = this.keys; const values = mapped(ids, id => freeze([id, this.get(id)])); let index = 0; return { next: () => index < values.length ? { value: values[index++], done: false } : { done: true } }; });

    for (let i = 0, names = ["FreeValueDataArray", "VibratoPointDataArray"]; i < names.length; ++i) {
        const kind = names[i]; const prototype = prototypes[kind] = create(null);
        property(prototype, "length", function () { return readRaw(state(this), "size"); });
        property(prototype, "role", function () { const role = readRaw(state(this), "role"); return kind === "FreeValueDataArray" ? ["original", "transform", "edited"][role] : ["amplitude", "frequency"][role]; });
        if (kind === "FreeValueDataArray") {
            property(prototype, "parameter", function () { return wrap(readRaw(state(this), "parameter")); });
            property(prototype, "step", () => 5);
            method(prototype, "at", function (index) { V.int(index); const values = this.toArray(); return index < values.length ? values[index] : undefined; });
        } else {
            method(prototype, "toOpenDSPX", function () { return toDSPX(this); }); method(prototype, "fromOpenDSPX", function (input) { fromDSPX(this, input); });
        }
        method(prototype, "toArray", function () { const values = readRaw(state(this), "items"); return freeze(mapped(values, value => value === undefined ? null : kind === "FreeValueDataArray" ? value : freeze({ x: value.x, y: value.y }))); });
        method(prototype, Symbol.iterator, function () { const values = this.toArray(); let index = 0; return { next: () => index < values.length ? { value: values[index++], done: false } : { done: true } }; });
        method(prototype, "splice", function (index, count, input = []) {
            const values = V.copy(input); V.array(values);
            const entry = requireKind(this, kind); if (kind === "FreeValueDataArray" && this.role === "original") throw new Type("Original samples are read-only.");
            const length = this.length; V.int(index, 0, length); V.int(count, 0, length - index);
            if (kind === "FreeValueDataArray") { V.each(values, value => { if (value !== null) V.int(value, -2147483648); }); V.int((length - count + values.length) * 5); }
            else V.validate("VibratoPointDataArray", values);
            mutation(entry, () => result(bridge.splice(entry.raw, index, count, values)));
        });
        method(prototype, "rotate", rotateItems(kind));
    }

    const deepFreeze = value => {
        if (!value || typeof value !== "object") return value;
        const names = keys(value); for (let i = 0; i < names.length; ++i) deepFreeze(value[names[i]]);
        return freeze(value);
    };
    const transactionValue = token => freeze({
        get active() { return result(bridge.transaction(token, "active", "")); },
        commit(name) { V.nonempty(name); if (!name.trim().length) throw new Type("A non-blank transaction name is required."); result(bridge.transaction(token, "commit", name)); },
        abort() { result(bridge.transaction(token, "abort", "")); },
    });
    const history = document => {
        const value = create(null);
        const read = () => command(document, "history");
        const names = ["position", "length", "cleanPosition", "canUndo", "canRedo", "undoName", "redoName"];
        for (let i = 0; i < names.length; ++i) { const key = names[i]; property(value, key, () => read()[key]); }
        value.undo = () => { const current = read(); if (!current.canUndo) return false; command(document, "goToHistory", [current.position - 1]); return true; };
        value.redo = () => { const current = read(); if (!current.canRedo) return false; command(document, "goToHistory", [current.position + 1]); return true; };
        value.goTo = position => { V.int(position, 0, read().length); command(document, "goToHistory", [position]); };
        return freeze(value);
    };
    const clipboard = document => {
        const value = create(null);
        property(value, "canCopy", () => command(document, "documentState").canCopy);
        property(value, "canPaste", () => command(document, "documentState").canPaste);
        const reference = tick => {
            if (tick !== undefined) return V.int(tick);
            const project = application.project;
            return project && project.document === document ? project.timeline.playhead : 0;
        };
        value.copy = tick => { command(document, "copy", [reference(tick)]); };
        value.cut = tick => { command(document, "cut", [reference(tick)]); };
        value.paste = (tick, mode = "currentPosition") => { V.oneOf(mode, ["currentPosition", "relativeToCopiedPlayhead", "originalPosition"]); command(document, "paste", [reference(tick), mode === "currentPosition" ? 0 : mode === "relativeToCopiedPlayhead" ? 1 : 2]); };
        return freeze(value);
    };
    const documentPrototype = prototypes.Document = create(null);
    property(documentPrototype, "model", function () { return wrap(readRaw(state(this), "model")); });
    property(documentPrototype, "selection", function () { return wrap(readRaw(state(this), "selectionModel")); });
    property(documentPrototype, "history", function () { return history(this); });
    property(documentPrototype, "clipboard", function () { return clipboard(this); });
    for (let i = 0, names = ["transactionActive", "canBounceSelection", "selectedClipSingerLayoutIdentical"]; i < names.length; ++i) { const name = names[i]; property(documentPrototype, name, function () { return command(this, "documentState")[name]; }); }
    method(documentPrototype, "beginTransaction", function () { return transactionValue(result(bridge.begin(state(this).raw))); });
    method(documentPrototype, "deleteSelection", function () { command(this, "deleteSelection"); });
    method(documentPrototype, "splitSelectionAt", function (tick) { V.int(tick); command(this, "splitSelectionAt", [tick]); });
    method(documentPrototype, "bounceSelectionToClip", function () { command(this, "bounceSelectionToClip"); });
    method(documentPrototype, "toOpenDSPX", function () { return toDSPX(this); });
    property(prototypes.DspxModel, "document", function () { return wrap(readRaw(state(this), "document")); });

    const projectPrototype = prototypes.Project = create(null);
    property(projectPrototype, "closed", function () { const entry = state(this, false); return !bridge.valid(entry.raw, entry.generation); });
    for (let i = 0, names = ["id", "displayName", "filePath", "modified", "window", "document", "timeline"]; i < names.length; ++i) {
        const name = names[i]; property(projectPrototype, name, function () { const value = readRaw(state(this), name); return name === "document" || name === "timeline" ? wrap(value) : value; });
    }
    property(projectPrototype, "storage", function () { const project = this; state(project); return freeze({ session(id) { return command(project, "sessionStorage", [V.nonempty(id)]); } }); });
    method(projectPrototype, "save", function () { return command(this, "save"); });
    method(projectPrototype, "saveAs", function (target) { return command(this, "saveAs", [target]); });
    method(projectPrototype, "saveCopy", function (target) { return command(this, "saveCopy", [target]); });
    method(projectPrototype, "close", function () { return command(this, "close"); });
    method(projectPrototype, "triggerAction", function (id) { return command(this, "triggerAction", [V.string(id)]); });

    const parameterDescriptor = raw => {
        if (raw === null) return null;
        const value = create(null);
        const conversion = raw.conversion;
        const names = keys(raw);
        for (let i = 0; i < names.length; ++i) if (names[i] !== "conversion") value[names[i]] = raw[names[i]];
        value.fromDSPXValue = input => { active(); if (input !== null) V.int(input, -2147483648); return result(conversion.convert("fromDSPXValue", input)); };
        value.toDSPXValue = input => { active(); if (typeof input !== "number") throw new Type("A number is required."); return result(conversion.convert("toDSPXValue", input)); };
        value.toDisplayValue = input => { active(); if (input !== null && typeof input !== "number") throw new Type("A number or null is required."); return result(conversion.convert("toDisplayValue", input)); };
        value.fromDisplayValue = input => { active(); V.number(input); return result(conversion.convert("fromDisplayValue", input)); };
        value.formatValue = input => { active(); if (input !== null && typeof input !== "number") throw new Type("A number or null is required."); return result(conversion.convert("formatValue", input)); };
        return freeze(value);
    };
    const singers = freeze({
        architectures() { const ids = result(bridge.service("architectures", [])); return freeze(mapped(ids, id => singers.architecture(id))); },
        architecture(id) { V.string(id); const value = result(bridge.service("architecture", [id])); if (value === null) return null; value.defaultExtra = V.copy(value.defaultExtra); value.parameters = freeze(mapped(value.parameters, parameterDescriptor)); return deepFreeze(value); },
        singers(architectureId) { V.string(architectureId); return freeze(mapped(result(bridge.service("singers", [architectureId])), id => singers.singer(architectureId, id))); },
        singer(architectureId, id) { V.string(architectureId); V.string(id); return deepFreeze(V.copy(result(bridge.service("singer", [architectureId, id])))); },
        parameter(architectureId, id, transform = false) { V.string(architectureId); V.string(id); V.boolean(transform); return parameterDescriptor(result(bridge.service("parameter", [architectureId, id, transform]))); },
    });
    const lyricDefaults = freeze({ lyricFor(clip) { const entry = state(clip); if (entry.kind !== "SingingClip") throw new Type("A singing clip is required."); return result(bridge.service("lyric", [entry.raw])); } });
    const trackColors = freeze({ get colors() { return deepFreeze(V.copy(result(bridge.service("trackColors", [])))); } });
    const projectOptions = options => {
        const value = options === undefined ? {} : V.copy(options, true); V.object(value);
        const names = keys(value); for (let i = 0; i < names.length; ++i) V.oneOf(names[i], ["defaultName", "model"]);
        if (own(value, "defaultName")) V.string(value.defaultName);
        return value;
    };
    const application = freeze({
        get project() { return result(bridge.service("project", [])); },
        get projects() { return freeze(mapped(result(bridge.service("projects", [])), wrap)); },
        get homeWindow() { return result(bridge.service("homeWindow", [])); },
        newProject(options) {
            const value = projectOptions(options);
            if (own(value, "model")) return wrap(result(bridge.service("newProjectFromOpenDSPX", [initData("DspxModel", value.model), defaulted(value, "defaultName", ""), true])));
            return wrap(result(bridge.service("newProject", [defaulted(value, "defaultName", "")])));
        },
        newProjectFromOpenDSPX(input, options) { const data = V.validate("DspxModel", V.copy(input)); const value = projectOptions(options); if (own(value, "model")) throw new Type("A model option is not supported for this operation."); return wrap(result(bridge.service("newProjectFromOpenDSPX", [data, defaulted(value, "defaultName", ""), false]))); },
        newProjectFromTemplate(file, options) { const value = projectOptions(options); if (own(value, "model")) throw new Type("A model option is not supported for this operation."); return wrap(result(bridge.service("newProjectFromTemplate", [file, defaulted(value, "defaultName", "")]))); },
        openProject(file) { return wrap(result(bridge.service("openProject", [file]))); },
        showHome() { result(bridge.service("showHome", [])); },
    });

    const selectionTypes = [null, "anchorNode", "clip", "label", "note", "tempo", "track", "keySignature", "dynamicMixingAnchor"];
    const viewProperties = { anchorNode: "anchorNodeSelectionModel", clip: "clipSelectionModel", label: "labelSelectionModel", note: "noteSelectionModel", tempo: "tempoSelectionModel", track: "trackSelectionModel", keySignature: "keySignatureSelectionModel", dynamicMixingAnchor: "dynamicMixingAnchorSelectionModel" };
    const selectionCode = type => { for (let i = 1; i < selectionTypes.length; ++i) if (selectionTypes[i] === type) return i; throw new Type("The selection type is not supported."); };
    const selectionOf = value => {
        const entry = state(value);
        const model = wrap(result(bridge.model(entry.raw)));
        return model.document.selection;
    };
    const selectionItemType = entry => {
        const map = { AnchorNode: "anchorNode", AudioClip: "clip", SingingClip: "clip", Label: "label", Note: "note", Tempo: "tempo", Track: "track", KeySignature: "keySignature", DynamicMixingAnchor: "dynamicMixingAnchor" };
        if (!map[entry.kind]) throw new Type("The entity cannot be selected.");
        return map[entry.kind];
    };
    const selectionPrototype = prototypes.Selection = create(null);
    property(selectionPrototype, "type", function () { return selectionTypes[readRaw(state(this), "selectionType")]; });
    property(selectionPrototype, "kind", function () { return this.freeParameter.active ? "freeParameter" : this.type; });
    property(selectionPrototype, "current", function () { return wrap(readRaw(state(this), "currentItem")); });
    property(selectionPrototype, "count", function () { return readRaw(state(this), "selectedCount"); });
    property(selectionPrototype, "activeView", function () { const type = this.type; return type === null ? null : this.view(type); });
    property(selectionPrototype, "freeParameter", function () { const model = wrap(result(bridge.model(state(this).raw))); return wrap(readRaw(state(model.document), "freeParameterSelectionModel")); });
    method(selectionPrototype, "view", function (type) { selectionCode(type); return wrap(readRaw(state(this), viewProperties[type])); });
    method(selectionPrototype, "items", function (type) { return this.view(type).items; });
    method(selectionPrototype, "currentItem", function (type) { return this.view(type).current; });
    method(selectionPrototype, "has", function (item) { const view = this.activeView; return view ? view.has(item) : false; });
    const selectionChange = (selection, item, flags) => {
        const target = state(selection); const entry = state(item); selectionItemType(entry);
        if (!bridge.sameDocument(target.raw, entry.raw)) throw new Type("The selected entity must belong to this document.");
        command(selection, "selection", [entry.raw, flags, 0, null]);
    };
    method(selectionPrototype, "select", function (item, options) { const value = options === undefined ? {} : V.copy(options, true); V.object(value); if (own(value, "clear")) V.boolean(value.clear); if (own(value, "current")) V.boolean(value.current); selectionChange(this, item, 1 | (value.clear ? 4 : 0) | (value.current === false ? 0 : 8)); });
    method(selectionPrototype, "deselect", function (item) { const entry = weakGet(states, item); if (entry && bridge.valid(entry.raw, entry.generation) && this.has(item)) selectionChange(this, item, 2); });
    method(selectionPrototype, "toggle", function (item) { selectionChange(this, item, 3 | 8); });
    method(selectionPrototype, "setCurrent", function (item) { selectionChange(this, item, 8); });
    method(selectionPrototype, "clearCurrent", function (type) { if (type === undefined) type = this.type; if (type === null) return; const code = selectionCode(type); const context = this.view(type).context; command(this, "selection", [null, 8, code, context ? state(context).raw : null]); });
    method(selectionPrototype, "clear", function (type) { const current = this.type; if (type !== undefined) selectionCode(type); if (current === null || (type !== undefined && type !== current)) return; command(this, "selection", [null, 4 | 8, selectionCode(current), null]); });
    method(selectionPrototype, "setContext", function (type, context) {
        const code = selectionCode(type); const entry = state(context);
        const expected = type === "note" ? "NoteSequence" : type === "anchorNode" ? "AnchorNodeSequence" : type === "dynamicMixingAnchor" ? "DynamicMixingAnchorSequence" : "";
        if (entry.kind !== expected || !bridge.sameDocument(state(this).raw, entry.raw)) throw new Type("The selection sequence is not compatible with this document.");
        command(this, "selection", [null, 4 | 8, code, entry.raw]);
    });
    method(selectionPrototype, "selectAll", function (type, context) {
        selectionCode(type);
        let items = [];
        if (type === "note" || type === "anchorNode" || type === "dynamicMixingAnchor") { this.setContext(type, context); items = context.toArray(); }
        else {
            const model = wrap(result(bridge.model(state(this).raw)));
            if (type === "clip") V.each(model.tracks.toArray(), track => append(items, track.clips.toArray()));
            else items = type === "track" ? model.tracks.toArray() : type === "tempo" ? model.tempos.toArray() : type === "label" ? model.labels.toArray() : model.keySignatures.toArray();
            command(this, "selection", [null, 4 | 8, selectionCode(type), null]);
        }
        for (let i = 0; i < items.length; ++i) selectionChange(this, items[i], 1 | 8);
    });
    const plural = { anchorNode: "anchorNodes", clip: "clips", label: "labels", note: "notes", tempo: "tempos", track: "tracks", keySignature: "keySignatures", dynamicMixingAnchor: "dynamicMixingAnchors" };
    for (let i = 1; i < selectionTypes.length; ++i) {
        const type = selectionTypes[i];
        property(selectionPrototype, plural[type], function () { return this.view(type); });
        const native = type === "keySignature" ? "KeySignatureSelectionModel" : type === "dynamicMixingAnchor" ? "DynamicMixingAnchorSelectionModel" : type === "anchorNode" ? "AnchorNodeSelectionModel" : type[0].toUpperCase() + type.slice(1) + "SelectionModel";
        const prototype = prototypes[native] = create(null);
        property(prototype, "type", () => type);
        property(prototype, "active", function () { return selectionOf(this).type === type; });
        property(prototype, "current", function () { return wrap(readRaw(state(this), "currentItem")); });
        property(prototype, "items", function () { return freeze(mapped(readRaw(state(this), "selectedItems"), wrap)); });
        property(prototype, "count", function () { return readRaw(state(this), "selectedCount"); });
        method(prototype, "has", function (item) { const raw = unwrap(item); if (!raw) return false; const items = readRaw(state(this), "selectedItems"); for (let j = 0; j < items.length; ++j) if (items[j] === raw) return true; return false; });
        const contexts = { note: "noteSequenceWithSelectedItems", anchorNode: "anchorNodeSequenceWithSelectedItems", dynamicMixingAnchor: "dynamicMixingAnchorSequenceWithSelectedItems" };
        if (contexts[type]) property(prototype, "context", function () { return wrap(readRaw(state(this), contexts[type])); });
        if (type === "clip") {
            property(prototype, "singingClipCount", function () { return readRaw(state(this), "selectedSingingClipCount"); });
            property(prototype, "audioClipCount", function () { return readRaw(state(this), "selectedAudioClipCount"); });
            property(prototype, "sequences", function () { return freeze(mapped(readRaw(state(this), "clipSequencesWithSelectedItems"), wrap)); });
        }
    }
    const freePrototype = prototypes.FreeParameterSelection = create(null);
    property(freePrototype, "active", function () { return readRaw(state(this), "active"); });
    property(freePrototype, "hasRange", function () { return readRaw(state(this), "hasSelection"); });
    property(freePrototype, "clip", function () { return this.active ? wrap(readRaw(state(this), "singingClip")) : null; });
    for (let i = 0, names = ["parameterId", "displayName"]; i < names.length; ++i) { const key = names[i]; property(freePrototype, key, function () { return this.active ? readRaw(state(this), key) : null; }); }
    property(freePrototype, "layer", function () { return this.active ? readRaw(state(this), "layer") === 0 ? "edited" : "transform" : null; });
    property(freePrototype, "start", function () { return this.hasRange ? readRaw(state(this), "start") : null; });
    property(freePrototype, "end", function () { return this.hasRange ? readRaw(state(this), "end") : null; });
    method(freePrototype, "setContext", function (clip, id, layer, displayName = "") { const entry = state(clip); if (entry.kind !== "SingingClip" || !bridge.sameDocument(state(this).raw, entry.raw)) throw new Type("The clip must belong to this document."); V.nonempty(id); V.oneOf(layer, ["edited", "transform"]); V.string(displayName); command(this, "freeContext", [entry.raw, id, displayName, layer === "edited" ? 0 : 1]); });
    method(freePrototype, "setRange", function (start, end) { V.int(start); V.int(end, start); if (!this.active) throw new CoreError({ code: "invalidState", operation: "setRange", message: "A parameter context is required." }); command(this, "freeRange", [start, end]); });
    method(freePrototype, "clearRange", function () { command(this, "clearFreeRange"); });
    method(freePrototype, "clearContext", function () { command(this, "clearFreeContext"); });

    const timelinePrototype = prototypes.Timeline = create(null);
    property(timelinePrototype, "ticksPerQuarter", () => 480);
    for (let i = 0, names = ["playhead", "previousPlayhead", "contentEndHint"]; i < names.length; ++i) {
        const name = names[i]; const native = name === "playhead" ? "position" : name === "previousPlayhead" ? "lastPosition" : "rangeHint";
        property(timelinePrototype, name, function () { return readRaw(state(this), native); }, function (value) { V.int(value, 0, name === "contentEndHint" ? V.max : V.max - 1); command(this, "timelineSet", [name, value]); });
    }
    method(timelinePrototype, "goTo", function (tick) { V.int(tick, 0, V.max - 1); command(this, "goTo", [tick]); });
    const maps = timeline => command(timeline, "timelineMaps");
    const signatures = timeline => {
        const values = maps(timeline).timeSignatures;
        if (!values.length || values[0].index !== 0) { const initial = [{ index: 0, numerator: 4, denominator: 4 }]; return append(initial, values); }
        return values;
    };
    method(timelinePrototype, "timeSignatureAt", function (measure) { V.int(measure); const values = signatures(this); let active = values[0]; for (let i = 1; i < values.length && values[i].index <= measure; ++i) active = values[i]; return freeze({ numerator: active.numerator, denominator: active.denominator }); });
    method(timelinePrototype, "toTick", function (input) {
        const position = V.copy(input); V.object(position); V.int(position.measure); V.int(position.beat); V.int(position.tick);
        const values = signatures(this); let tick = 0; let previous = values[0];
        for (let i = 1; i < values.length && values[i].index <= position.measure; ++i) { tick += (values[i].index - previous.index) * (1920 * previous.numerator / previous.denominator); previous = values[i]; }
        return V.int(tick + (position.measure - previous.index) * (1920 * previous.numerator / previous.denominator) + position.beat * (1920 / previous.denominator) + position.tick);
    });
    method(timelinePrototype, "toMusicalPosition", function (tick) {
        V.int(tick); const values = signatures(this); let begin = 0; let signature = values[0];
        for (let i = 1; i < values.length; ++i) { const next = begin + (values[i].index - signature.index) * (1920 * signature.numerator / signature.denominator); if (tick < next) break; begin = next; signature = values[i]; }
        const perBeat = 1920 / signature.denominator; const perBar = perBeat * signature.numerator; const offset = tick - begin;
        return freeze({ measure: signature.index + V.floor(offset / perBar), beat: V.floor((offset % perBar) / perBeat), tick: offset % perBeat });
    });
    const tempos = timeline => { const values = maps(timeline).tempos; return !values.length || values[0].position !== 0 ? append([{ position: 0, value: 120 }], values) : values; };
    method(timelinePrototype, "tempoAt", function (tick) { V.int(tick); const values = tempos(this); let tempo = values[0].value; for (let i = 1; i < values.length && values[i].position <= tick; ++i) tempo = values[i].value; return tempo; });
    method(timelinePrototype, "toMilliseconds", function (tick) { V.int(tick); const values = tempos(this); let ms = 0; let previous = values[0]; for (let i = 1; i < values.length && values[i].position <= tick; ++i) { ms += (values[i].position - previous.position) * 60000 / (previous.value * 480); previous = values[i]; } return ms + (tick - previous.position) * 60000 / (previous.value * 480); });
    method(timelinePrototype, "fromMilliseconds", function (ms) { V.number(ms, 0); const values = tempos(this); let elapsed = 0; let previous = values[0]; for (let i = 1; i < values.length; ++i) { const duration = (values[i].position - previous.position) * 60000 / (previous.value * 480); if (ms < elapsed + duration) break; elapsed += duration; previous = values[i]; } return V.int(previous.position + round((ms - elapsed) * previous.value * 480 / 60000)); });

    freeze(entityPrototype); freeze(collectionPrototype);
    const prototypeNames = keys(prototypes);
    for (let i = 0; i < prototypeNames.length; ++i) freeze(prototypes[prototypeNames[i]]);
    return freeze({ exports: freeze({ CoreError, application, singers, lyricDefaults, trackColors }), bridge, wrap, unwrap });
})
