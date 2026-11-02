const { application } = require("diffscope:core");
const { dialogs } = require("diffscope:shell");
const { loadI18n, t } = require("diffscope:runtime");

loadI18n("i18n");

function showSelectionWarning(project) {
    dialogs.alert(t("transpose.selectionWarning"), {
        title: t("transpose.title"),
        severity: "warning",
        parent: project.window
    });
}

function showRangeWarning(project) {
    dialogs.alert(t("transpose.rangeWarning"), {
        title: t("transpose.rangeWarningTitle"),
        severity: "warning",
        parent: project.window
    });
}

module.exports = defineScript({
    id: "org.diffscope.transpose",
    name: t("transpose.title"),
    version: "1.0.0",
    description: t("transpose.description"),
    actions: [
        {
            name: t("transpose.title"),
            description: t("transpose.description"),
            requiresProject: true,
            execute() {
                const project = application.project;
                const document = project.document;
                const selection = document.selection;
                const notes = new Map();

                if (selection.type === "clip") {
                    const clips = selection.clips.items;
                    if (clips.length < 1 || clips.some(clip => clip.kind !== "singing")) {
                        showSelectionWarning(project);
                        return;
                    }
                    for (const clip of clips) {
                        for (const note of clip.notes) {
                            notes.set(note.handle, note);
                        }
                    }
                } else if (selection.type === "note") {
                    const selectedNotes = selection.notes.items;
                    if (selectedNotes.length < 1) {
                        showSelectionWarning(project);
                        return;
                    }
                    for (const note of selectedNotes) {
                        notes.set(note.handle, note);
                    }
                } else {
                    showSelectionWarning(project);
                    return;
                }

                const result = dialogs.form({
                    title: t("transpose.title"),
                    description: t("transpose.formDescription"),
                    fields: [
                        {
                            type: "integer",
                            name: "semitone",
                            label: t("transpose.semitoneLabel"),
                            value: 0,
                            minimum: -127,
                            maximum: 127,
                            step: 1,
                            control: "spinBox",
                            required: true
                        }
                    ],
                    parent: project.window
                });
                if (result === null) {
                    return;
                }

                const semitone = result.semitone;
                const transaction = document.beginTransaction();

                try {
                    if (semitone === 0 || notes.size === 0) {
                        transaction.abort();
                        return;
                    }

                    for (const note of notes.values()) {
                        const keyNumber = note.keyNumber + semitone;
                        if (keyNumber < 0 || keyNumber > 127) {
                            transaction.abort();
                            showRangeWarning(project);
                            return;
                        }
                    }

                    for (const note of notes.values()) {
                        note.keyNumber = note.keyNumber + semitone;
                    }

                    transaction.commit(t("transpose.title"));
                } catch (error) {
                    if (transaction.active) {
                        transaction.abort();
                    }
                    throw error;
                }
            }
        }
    ]
});
