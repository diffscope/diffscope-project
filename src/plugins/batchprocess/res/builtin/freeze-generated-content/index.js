const { application } = require("diffscope:core");
const { dialogs } = require("diffscope:shell");
const { loadI18n, t } = require("diffscope:runtime");

loadI18n("i18n");

function showSelectionWarning(project) {
    dialogs.alert(t("freezeGeneratedContent.selectionWarning"), {
        title: t("freezeGeneratedContent.title"),
        severity: "warning",
        parent: project.window
    });
}

function freezeParameter(parameter) {
    const original = parameter.original.toArray();
    const edited = parameter.freeEdited;
    const samples = edited.toArray().slice();
    let changed = false;
    for (let index = 0; index < original.length; ++index) {
        const value = original[index];
        if (value === null || (index < samples.length && samples[index] !== null)) {
            continue;
        }
        while (samples.length < index) {
            samples.push(null);
        }
        samples[index] = value;
        changed = true;
    }
    if (changed) {
        edited.splice(0, edited.length, samples);
    }
    return changed;
}

module.exports = defineScript({
    id: "org.diffscope.freeze-generated-content",
    name: t("freezeGeneratedContent.title"),
    version: "1.0.0",
    description: t("freezeGeneratedContent.description"),
    actions: [
        {
            name: t("freezeGeneratedContent.title"),
            description: t("freezeGeneratedContent.description"),
            requiresProject: true,
            execute() {
                const project = application.project;
                const document = project.document;
                const selection = document.selection;
                const notes = new Map();
                let clips = [];

                if (selection.type === "clip") {
                    clips = selection.clips.items.filter(clip => clip.kind === "singing");
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

                if (clips.length === 0 && notes.size === 0) {
                    showSelectionWarning(project);
                    return;
                }

                const fields = [];
                if (clips.length !== 0) {
                    fields.push({
                        type: "boolean",
                        name: "parameters",
                        label: t("freezeGeneratedContent.contents.parameters"),
                        value: true
                    });
                }
                fields.push({
                    type: "boolean",
                    name: "phonemes",
                    label: t("freezeGeneratedContent.contents.phonemes"),
                    value: true
                });
                fields.push({
                    type: "boolean",
                    name: "pronunciation",
                    label: t("freezeGeneratedContent.contents.pronunciation"),
                    value: true
                });

                const readLock = document.mustAcquireReadLock();
                const result = dialogs.form({
                    title: t("freezeGeneratedContent.title"),
                    fields,
                    parent: project.window
                });
                readLock.release();
                if (result === null) {
                    return;
                }

                const transaction = document.beginTransaction();
                let changed = false;
                if (result.parameters) {
                    for (const clip of clips) {
                        for (const [, parameter] of clip.parameters) {
                            if (freezeParameter(parameter)) {
                                changed = true;
                            }
                        }
                    }
                }
                if (result.phonemes) {
                    for (const note of notes.values()) {
                        if (note.editedPhonemes.length === 0 && note.originalPhonemes.length !== 0) {
                            note.editedPhonemes.fromOpenDSPX(note.originalPhonemes.toOpenDSPX());
                            changed = true;
                        }
                    }
                }
                if (result.pronunciation) {
                    for (const note of notes.values()) {
                        const pronunciation = note.originalPronunciation;
                        if (note.editedPronunciation === "" && pronunciation !== "") {
                            note.editedPronunciation = pronunciation;
                            changed = true;
                        }
                    }
                }

                if (!changed) {
                    transaction.abort();
                    return;
                }

                transaction.commit(t("freezeGeneratedContent.title"));
            }
        }
    ]
});
