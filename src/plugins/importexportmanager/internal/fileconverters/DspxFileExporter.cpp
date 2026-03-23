#include "DspxFileExporter.h"

#include <sstream>

#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QWindow>
#include <QComboBox>
#include <QLoggingCategory>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/model.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/OpenSaveProjectFileScenario.h>

#include <opendspxserializer/serializer.h>

namespace ImportExportManager::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcDspxFileExporter, "diffscope.importexportmanager.dspxfileexporter");

    DspxFileExporter::DspxFileExporter(QObject *parent) : FileConverter(parent) {
        setName(tr("DSPX"));
        setDescription(tr("Export as DSPX file compatible with older versions of DiffScope and other editors"));
        setFileDialogFilters({tr("DiffScope Project Exchange Format (*.dspx)")});
        setMode(Export);
        setHeuristicFilters({QStringLiteral("*.dspx")});
    }

    DspxFileExporter::~DspxFileExporter() = default;

    bool DspxFileExporter::execExport(const QString &path, const opendspx::Model &model_, QWindow *window) {
        Core::OpenSaveProjectFileScenario scenario;
        scenario.setWindow(window);
        QDialog dlg;
        dlg.setWindowTitle(tr("Export DSPX"));
        auto layout = new QVBoxLayout;
        auto formLayout = new QFormLayout;
        auto versionComboBox = new QComboBox;
        versionComboBox->addItem(QStringLiteral("1.0.0"), QVariant::fromValue(opendspx::Model::Version::V1));
        formLayout->addRow(tr("Format version"), versionComboBox);
        auto keepWorkspaceDataCheckBox = new QCheckBox(tr("Keep Workspace data"));
        keepWorkspaceDataCheckBox->setChecked(true);
        formLayout->addRow(keepWorkspaceDataCheckBox);
        auto workspaceInstruction = new QLabel(tr("Workspace data is not part of the DSPX standard and is editor-specific. Different editors may interpret the same fields differently, which can potentially lead to unexpected behavior or compatibility issues."));
        workspaceInstruction->setWordWrap(true);
        formLayout->addRow(workspaceInstruction);
        auto compressCheckbox = new QCheckBox(tr("Compress"));
        compressCheckbox->setChecked(true);
        formLayout->addRow(compressCheckbox);
        layout->addLayout(formLayout);
        layout->addStretch();
        auto buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        auto button = new QPushButton(tr("Export"));
        button->setDefault(true);
        connect(button, &QPushButton::clicked, &dlg, &QDialog::accept);
        buttonLayout->addWidget(button);
        layout->addLayout(buttonLayout);
        dlg.setLayout(layout);
        dlg.setModal(true);
        if (dlg.exec() != QDialog::Accepted) {
            return false;
        }
        auto model = model_;
        model.version = versionComboBox->currentData().value<opendspx::Model::Version>();
        qCDebug(lcDspxFileExporter) << "Keep Workspace data:" << keepWorkspaceDataCheckBox->isChecked();
        if (!keepWorkspaceDataCheckBox->isChecked()) {
            model.content.workspace.clear();
            for (auto &track : model.content.tracks) {
                track.workspace.clear();
                for (auto &clip : track.clips) {
                    clip->workspace.clear();
                    if (clip->type != opendspx::Clip::Type::Singing) {
                        continue;
                    }
                    auto singingClip = std::static_pointer_cast<opendspx::SingingClip>(clip);
                    for (auto &note : singingClip->notes) {
                        note.workspace.clear();
                    }
                }
            }
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCCritical(lcDspxFileExporter) << "Failed to write file:" << path << file.errorString();
            scenario.showSaveFailMessageBox(path, file.errorString());
            return false;
        }
        opendspx::SerializationErrorList errors;
        std::stringstream out(std::ios::out);
        opendspx::Serializer::serialize(out, model, errors, opendspx::Serializer::CheckError, compressCheckbox->isChecked());
        auto data = QByteArray::fromStdString(out.str());
        auto bytesWritten = file.write(data);
        if (bytesWritten != data.size()) {
            qCCritical(lcDspxFileExporter) << "Failed to write file:" << path << file.errorString();
            scenario.showSaveFailMessageBox(path, file.errorString());
            return false;
        }
        file.close();
        return true;
    }

}
