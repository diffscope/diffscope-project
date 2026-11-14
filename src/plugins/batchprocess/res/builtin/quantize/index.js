const { application } = require("diffscope:core");
const { dialogs } = require("diffscope:shell");
const { loadI18n, t } = require("diffscope:runtime");

loadI18n("i18n");

const ticksPerWhole = 1920;

function roundToNearestMultiple(value, grid) {
    const lower = Math.floor(value / grid) * grid;
    const upper = lower + grid;
    return value - lower <= upper - value ? lower : upper;
}

function snapToGrid(value, grid, timeline) {
    if (value < 0) {
        return roundToNearestMultiple(value, grid);
    }

    const measure = timeline.toMusicalPosition(value).measure;
    const start = timeline.toTick({ measure, beat: 0, tick: 0 });
    const timeSignature = timeline.timeSignatureAt(measure);
    const length = ticksPerWhole / timeSignature.denominator * timeSignature.numerator;
    let lower;
    let upper;

    if (grid >= length) {
        const hop = Math.max(1, Math.floor(grid / length));
        const lowerMeasure = measure - measure % hop;
        lower = timeline.toTick({ measure: lowerMeasure, beat: 0, tick: 0 });
        upper = timeline.toTick({ measure: lowerMeasure + hop, beat: 0, tick: 0 });
    } else {
        lower = start + Math.floor((value - start) / grid) * grid;
        upper = Math.min(lower + grid, start + length);
    }

    return value - lower <= upper - value ? lower : upper;
}

function showSelectionWarning(project) {
    dialogs.alert(t("quantize.selectionWarning"), {
        title: t("quantize.title"),
        severity: "warning",
        parent: project.window
    });
}

module.exports = defineScript({
    id: "org.diffscope.quantize",
    name: t("quantize.title"),
    version: "1.0.0",
    description: t("quantize.description"),
    actions: [
        {
            name: t("quantize.title"),
            description: t("quantize.description"),
            requiresProject: true,
            execute() {
                const project = application.project;
                const document = project.document;
                let transaction = null;

                const selection = document.selection;
                const notes = new Map();

                if (selection.type === "clip") {
                    const clips = selection.clips.items.filter(clip => clip.kind === "singing");
                    if (clips.length === 0) {
                        showSelectionWarning(project);
                        return;
                    }
                    for (const clip of clips) {
                        for (const note of clip.notes) {
                            notes.set(note.handle, note);
                        }
                    }
                } else if (selection.type === "note") {
                    for (const note of selection.notes.items) {
                        notes.set(note.handle, note);
                    }
                }

                if (notes.size === 0) {
                    showSelectionWarning(project);
                    return;
                }

                const readLock = document.mustAcquireReadLock();
                const result = dialogs.form({
                    title: t("quantize.title"),
                    fields: [
                        // TODO use editor's snap options as default value
                        {
                            type: "choice",
                            name: "quantize",
                            label: t("quantize.gridLabel"),
                            items: [
                                { value: "1", label: t("quantize.notes.whole") },
                                { value: "2", label: t("quantize.notes.half") },
                                { value: "4", label: t("quantize.notes.quarter") },
                                { value: "8", label: t("quantize.notes.eighth") },
                                { value: "16", label: t("quantize.notes.sixteenth") },
                                { value: "32", label: t("quantize.notes.thirtySecond") },
                                { value: "64", label: t("quantize.notes.sixtyFourth") },
                                { value: "128", label: t("quantize.notes.oneHundredTwentyEighth") }
                            ],
                            value: "16",
                            required: true
                        },
                        {
                            type: "choice",
                            name: "tuplet",
                            label: t("quantize.rhythmLabel"),
                            items: [
                                { value: "normal", label: t("quantize.rhythms.normal") },
                                { value: "triplet", label: t("quantize.rhythms.triplet") },
                                { value: "quintuplet", label: t("quantize.rhythms.quintuplet") }
                            ],
                            value: "normal",
                            required: true
                        },
                        {
                            type: "choice",
                            name: "mode",
                            label: t("quantize.modeLabel"),
                            items: [
                                { value: "both", label: t("quantize.modes.both") },
                                { value: "start", label: t("quantize.modes.start") },
                                { value: "length", label: t("quantize.modes.length") }
                            ],
                            value: "both",
                            required: true
                        }
                    ],
                    parent: project.window
                });
                readLock.release();
                if (result === null) {
                    return;
                }

                const tuplet = result.tuplet === "triplet" ? 1.5 : result.tuplet === "quintuplet" ? 1.25 : 1;
                const grid = ticksPerWhole / (Number(result.quantize) * tuplet);
                const quantizeStart = result.mode !== "length";
                const quantizeLength = result.mode !== "start";
                const timeline = project.timeline;
                const changes = [];
                transaction = document.beginTransaction();

                for (const note of notes.values()) {
                    const sequence = note.noteSequence;
                    if (sequence === null) {
                        transaction.abort();
                        showSelectionWarning(project);
                        return;
                    }

                    const clipStart = sequence.clip.start;
                    let position = note.position;
                    let length = note.length;

                    if (quantizeStart) {
                        const absoluteStart = clipStart + position;
                        const snapped = snapToGrid(absoluteStart, grid, timeline);
                        position = Math.max(0, snapped - clipStart);
                    }

                    if (quantizeLength) {
                        length = Math.max(grid, roundToNearestMultiple(length, grid));
                    }

                    if (position !== note.position || length !== note.length) {
                        changes.push({ note, position, length });
                    }
                }

                if (changes.length === 0) {
                    transaction.abort();
                    return;
                }

                for (const { note, position, length } of changes) {
                    if (length < note.length) {
                        note.length = length;
                    }
                    if (position !== note.position) {
                        note.position = position;
                    }
                    if (length > note.length) {
                        note.length = length;
                    }
                }

                transaction.commit(t("quantize.title"));

            }
        }
    ]
});
