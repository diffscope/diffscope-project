const { application } = require("diffscope:core");
const { parse: parsePath } = require("diffscope:fs");
const { loadI18n, MethodLogicError, t } = require("diffscope:runtime");
const { dialogs } = require("diffscope:shell");
const { storage } = require("diffscope:storage");

loadI18n("i18n");

const lrcFileFilters = [{ name: t("lrcLabels.fileFilter"), patterns: ["*.lrc"] }];
const timestampPattern = /^\[(\d+):([0-5]\d)(?:\.(\d{1,3}))?\]/;
const wordTimestampPattern = /<\d+:[0-5]\d(?:\.\d{1,3})?>/g;
const metadataPattern = /^\[([a-z][a-z\d_-]*|#):(.*)\]$/i;
const karaokeFontSizeStorageId = "org.diffscope.lrc-labels";
const karaokeFontSizeStorageKey = "karaokeFontSize";
const karaokeFontSizeMinimum = 16;
const karaokeFontSizeMaximum = 64;
const karaokeFontSizeDefault = 48;

function parseLrc(contents) {
    const entries = [];
    let offset = 0;

    const lines = contents.split(/\r\n|\n|\r/);
    for (let index = 0; index < lines.length; ++index) {
        let text = lines[index].trim();
        if (text === "") {
            continue;
        }

        const metadata = metadataPattern.exec(text);
        if (metadata !== null) {
            if (metadata[1].toLowerCase() === "offset") {
                const value = metadata[2].trim();
                if (!/^[+-]?\d+$/.test(value)) {
                    return { errorLine: index + 1 };
                }
                offset = Number(value);
            }
            continue;
        }

        const times = [];
        let timestamp = timestampPattern.exec(text);
        while (timestamp !== null) {
            const fraction = timestamp[3] || "";
            const milliseconds = (Number(timestamp[1]) * 60 + Number(timestamp[2])) * 1000
                + Number(fraction) * 10 ** (3 - fraction.length);
            times.push(milliseconds);
            text = text.slice(timestamp[0].length).trim();
            timestamp = timestampPattern.exec(text);
        }

        text = text.replace(wordTimestampPattern, "").trim();
        if (times.length === 0 || /^\[\s*[+-]?\d+:/.test(text) || /<\s*[+-]?\d+:/.test(text)) {
            return { errorLine: index + 1 };
        }
        for (const milliseconds of times) {
            entries.push({ milliseconds, text });
        }
    }

    return { entries, offset };
}

function formatTimestamp(milliseconds) {
    const time = Math.round(milliseconds);
    const minutes = Math.floor(time / 60000);
    const seconds = Math.floor(time / 1000) % 60;
    const fraction = time % 1000;
    return `[${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}.${String(fraction).padStart(3, "0")}]`;
}

function promptStartTick(project, title, maximum = 2147483647) {
    const form = dialogs.form({
        title,
        fields: [
            {
                type: "musicTime",
                name: "startTick",
                label: t("lrcLabels.startPositionLabel"),
                value: Math.min(project.timeline.playhead, maximum),
                maximum
            }
        ],
        parent: project.window
    });
    return form === null ? null : form.startTick;
}

function importLrc() {
    const project = application.project;
    const file = dialogs.openFile({
        title: t("lrcLabels.import.openTitle"),
        filters: lrcFileFilters,
        parent: project.window
    });
    if (file === null) {
        return;
    }

    const title = t("lrcLabels.import.title");
    const startTick = promptStartTick(project, title);
    if (startTick === null) {
        return;
    }

    const result = parseLrc(file.readText());
    if (result.errorLine !== undefined || result.entries.length === 0) {
        const message = result.errorLine !== undefined
            ? t("lrcLabels.import.invalidLine", { line: result.errorLine.toLocaleString() })
            : t("lrcLabels.import.noTimestampedLines");
        dialogs.alert(message, {
            title: t("lrcLabels.import.failureTitle"),
            severity: "warning",
            parent: project.window
        });
        return;
    }

    const document = project.document;
    const labels = document.model.labels;
    const timeline = project.timeline;
    const startMilliseconds = timeline.toMilliseconds(startTick);
    const transaction = document.beginTransaction();
    for (const { milliseconds, text } of result.entries) {
        const projectMilliseconds = startMilliseconds + (milliseconds - result.offset);
        const position = timeline.fromMilliseconds(Math.max(0, projectMilliseconds));
        labels.create({ position, text });
    }
    transaction.commit(title);
}

function exportLrc() {
    const project = application.project;
    const title = t("lrcLabels.export.title");
    const labels = Array.from(project.document.model.labels);
    if (labels.length === 0) {
        dialogs.alert(t("lrcLabels.export.noLabels"), {
            title,
            severity: "warning",
            parent: project.window
        });
        return;
    }
    labels.sort((left, right) => left.position - right.position);

    const projectPath = project.filePath;
    const file = dialogs.saveFile({
        title: t("lrcLabels.export.saveTitle"),
        suggestedName: projectPath === null ? "labels.lrc" : `${parsePath(projectPath).name}.lrc`,
        filters: lrcFileFilters,
        parent: project.window
    });
    if (file === null) {
        return;
    }

    const startTick = promptStartTick(project, title, labels[0].position);
    if (startTick === null) {
        return;
    }

    const timeline = project.timeline;
    const startMilliseconds = timeline.toMilliseconds(startTick);
    const lines = labels.map(({ position, text }) => {
        const timestamp = formatTimestamp(timeline.toMilliseconds(position) - startMilliseconds);
        return timestamp + text.replace(/\r\n|\n|\r/g, " ");
    });
    file.writeText(lines.join("\n") + "\n");
}

function karaokeFontSizeStorage() {
    return storage.persistent(karaokeFontSizeStorageId);
}

function readKaraokeFontSize() {
    const value = Number(karaokeFontSizeStorage().getItem(karaokeFontSizeStorageKey));
    return Number.isInteger(value) && value >= karaokeFontSizeMinimum && value <= karaokeFontSizeMaximum
        ? value
        : karaokeFontSizeDefault;
}

function getKaraokeFontSize() {
    return readKaraokeFontSize();
}

function editKaraokeFontSize() {
    const project = application.project;
    const form = dialogs.form({
        title: t("lrcLabels.karaoke.fontSizeTitle"),
        fields: [
            {
                type: "integer",
                name: "fontSize",
                label: t("lrcLabels.karaoke.fontSizeLabel"),
                value: readKaraokeFontSize(),
                minimum: karaokeFontSizeMinimum,
                maximum: karaokeFontSizeMaximum,
                step: 1,
                required: true
            }
        ],
        parent: project.window
    });
    if (form === null) {
        return null;
    }
    karaokeFontSizeStorage().setItem(karaokeFontSizeStorageKey, String(form.fontSize));
    return form.fontSize;
}

function editKaraokeLyrics({ add, position }) {
    const project = application.project;
    const document = project.document;
    const labels = document.model.labels;
    const title = t(add ? "lrcLabels.karaoke.addTitle" : "lrcLabels.karaoke.editTitle");
    const fields = [];
    if (add) {
        fields.push({
            type: "musicTime",
            name: "position",
            label: t("lrcLabels.karaoke.positionLabel"),
            value: position
        });
    }

    const readLock = document.tryAcquireReadLock();
    if (readLock === null) {
        throw new MethodLogicError(t("lrcLabels.karaoke.projectBusy"));
    }

    let label;
    let originalText;
    let form;
    try {
        if (!add) {
            label = labels.toArray().filter(item => item.position === position).pop();
        }
        originalText = add ? "" : label.text;
        fields.push({
            type: "text",
            name: "text",
            label: t("lrcLabels.karaoke.textLabel"),
            value: originalText
        });
        form = dialogs.form({ title, fields, parent: project.window });
    } finally {
        readLock.release();
    }
    if (form === null || (!add && form.text === originalText)) {
        return;
    }

    const transaction = document.beginTransaction();
    if (add) {
        labels.create({ position: form.position, text: form.text });
    } else {
        label.text = form.text;
    }
    transaction.commit(title);
}

module.exports = defineScript({
    id: "org.diffscope.lrc-labels",
    name: t("lrcLabels.title"),
    version: "1.0.0",
    description: t("lrcLabels.description"),
    actions: [
        {
            name: t("lrcLabels.import.title"),
            description: t("lrcLabels.import.description"),
            requiresProject: true,
            execute: importLrc
        },
        {
            name: t("lrcLabels.export.title"),
            description: t("lrcLabels.export.description"),
            requiresProject: true,
            execute: exportLrc
        }
    ],
    methods: [
        {
            name: "karaoke.editLyrics",
            internal: true,
            params: [
                { name: "add", schema: { type: "boolean" }, required: true },
                { name: "position", schema: { type: "integer", minimum: 0, maximum: 2147483647 }, required: true }
            ],
            execute: editKaraokeLyrics
        },
        {
            name: "karaoke.fontSize",
            internal: true,
            params: [],
            result: {
                name: "fontSize",
                schema: { type: "integer", minimum: karaokeFontSizeMinimum, maximum: karaokeFontSizeMaximum }
            },
            annotations: { readOnlyHint: true },
            execute: getKaraokeFontSize
        },
        {
            name: "karaoke.editFontSize",
            internal: true,
            params: [],
            result: {
                name: "fontSize",
                schema: { type: ["integer", "null"], minimum: karaokeFontSizeMinimum, maximum: karaokeFontSizeMaximum }
            },
            execute: editKaraokeFontSize
        }
    ],
    panels: [
        {
            name: t("lrcLabels.karaoke.title"),
            description: "",
            component: "./KaraokeLyricsPanel.qml"
        }
    ]
});
