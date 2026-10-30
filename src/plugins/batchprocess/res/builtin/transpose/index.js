const { application } = require("diffscope:core");
const { dialogs } = require("diffscope:shell");

function showSelectionWarning(project) {
    dialogs.alert("Select one or more singing clips or one or more notes.", {
        title: "Transpose",
        severity: "warning",
        parent: project.window
    });
}

function showRangeWarning(project) {
    dialogs.alert("At least one note would be outside the valid key range from 0 through 127 after transportation.", {
        title: "Transposition Failed",
        severity: "warning",
        parent: project.window
    });
}

module.exports = defineScript({
    id: "org.diffscope.transpose",
    name: "Transpose",
    version: "1.0.0",
    description: "Transposes notes in the selected singing clips or the selected notes by a number of semitones.",
    actions: [
        {
            name: "Transpose",
            description: "Transposes notes in the selected singing clips or the selected notes by a number of semitones.",
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
                    title: "Transpose",
                    description: "Enter the number of semitones to shift each note. Use a negative value to transpose down.",
                    fields: [
                        {
                            type: "integer",
                            name: "semitone",
                            label: "Semitone",
                            value: 0,
                            minimum: -127,
                            maximum: 127,
                            step: 1,
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

                    transaction.commit("Transpose");
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
