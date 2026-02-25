#include "MIDIFileExporter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSet>
#include <QVBoxLayout>
#include <QSaveFile>
#include <QLoggingCategory>
#include <QDir>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <opendspx/model.h>
#include <opendspxconverter/midi/midiconverter.h>
#include <opendspxconverter/midi/midiintermediatedata.h>

#include <midiformatconverter/internal/MIDITextCodecConverter.h>

namespace MIDIFormatConverter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcMIDIFileExporter, "diffscope.midiformatconverter.midifileexporter")

    MIDIFileExporter::MIDIFileExporter(QObject *parent) : FileConverter(parent) {
        setName(tr("MIDI"));
        setDescription(tr("Export as Standard MIDI file"));
        setFileDialogFilters({tr("Standard MIDI File (*.mid *.midi *.smf)")});
        setMode(Export);
        setHeuristicFilters({QStringLiteral("*.mid"), QStringLiteral("*.midi"), QStringLiteral("*.smf")});
    }

    MIDIFileExporter::~MIDIFileExporter() = default;

    bool MIDIFileExporter::execExport(const QString &path, const QDspx::Model &model, QWindow *window) {
        QDialog dialog;
        dialog.setWindowTitle(tr("MIDI Export"));

        auto *mainLayout = new QVBoxLayout(&dialog);

        auto *optionsGroup = new QGroupBox(tr("Options"), &dialog);
        auto *optionsLayout = new QVBoxLayout(optionsGroup);

        auto *encodingLayout = new QHBoxLayout;
        auto *encodingLabel = new QLabel(tr("Encoding"), optionsGroup);
        auto *encodingComboBox = new QComboBox(optionsGroup);
        const QByteArray utf8Name = QStringConverter::nameForEncoding(QStringConverter::Utf8);
        int defaultEncodingIndex = -1;

        const auto codecs = MIDITextCodecConverter::availableCodecs();
        for (const auto &codec : codecs) {
            const int row = encodingComboBox->count();
            encodingComboBox->addItem(codec.displayName, codec.identifier);
            if (defaultEncodingIndex < 0 && codec.identifier == MIDITextCodecConverter::defaultCodec())
                defaultEncodingIndex = row;
        }

        if (defaultEncodingIndex >= 0)
            encodingComboBox->setCurrentIndex(defaultEncodingIndex);
        else if (encodingComboBox->count() > 0)
            encodingComboBox->setCurrentIndex(0);
        encodingLayout->addWidget(encodingLabel);
        encodingLayout->addWidget(encodingComboBox, 1);
        optionsLayout->addLayout(encodingLayout);

        auto *separateCheckBox = new QCheckBox(tr("Separate singing clips"), optionsGroup);
        optionsLayout->addWidget(separateCheckBox);

        optionsGroup->setLayout(optionsLayout);
        mainLayout->addWidget(optionsGroup);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        mainLayout->addWidget(buttonBox);

        if (dialog.exec() != QDialog::Accepted) {
            return false;
        }
        const QByteArray selectedEncoding = encodingComboBox->currentData().toByteArray();
        const bool separateClips = separateCheckBox->isChecked();

        auto intermediateData = QDspx::MidiConverter::convertDspxToIntermediate(model, [selectedEncoding](const QString &text) {
            return MIDITextCodecConverter::encode(text, selectedEncoding);
        }, {480, separateClips});
        auto data = QDspx::MidiConverter::convertIntermediateToMidi(intermediateData);
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            qCCritical(lcMIDIFileExporter) << "Failed to write file:" << path << file.errorString();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to save file"), QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), file.errorString()));
            return false;
        }
        file.write(data);
        if (!file.commit()) {
            qCCritical(lcMIDIFileExporter) << "Failed to commit file:" << path << file.errorString();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to save file"), QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), file.errorString()));
            return false;
        }
        return true;
    }

}
