#include "DspxFileImporter.h"

#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow>
#include <QLoggingCategory>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/model.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/OpenSaveProjectFileScenario.h>

#include <opendspxserializer/serializer.h>

namespace ImportExportManager::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcDspxFileImporter, "diffscope.importexportmanager.dspxfileimporter");

    DspxFileImporter::DspxFileImporter(QObject *parent) : FileConverter(parent) {
        setName(tr("DSPX"));
        setDescription(tr("Import DSPX file compatible with DiffScope"));
        setFileDialogFilters({tr("DiffScope Project Exchange Format (*.dspx)")});
        setMode(Import);
        setHeuristicFilters({QStringLiteral("*.dspx")});
    }

    DspxFileImporter::~DspxFileImporter() = default;

    bool DspxFileImporter::execImport(const QString &path, QDspx::Model &model, QWindow *window) {
        Core::OpenSaveProjectFileScenario scenario;
        scenario.setWindow(window);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCCritical(lcDspxFileImporter) << "Failed to read file:" << path << file.errorString();
            scenario.showOpenFailMessageBox(path, file.errorString());
            return false;
        }
        QDspx::SerializationErrorList errors;
        model = QDspx::Serializer::deserialize(file.readAll(), errors);
        file.close();
        if (errors.containsFatal() || errors.containsError()) {
            qCCritical(lcDspxFileImporter) << "Failed to deserialize file:" << path;
            scenario.showDeserializationFailMessageBox(path);
            return false;
        }
        QDialog dlg;
        dlg.setWindowTitle(tr("Import DSPX"));
        auto layout = new QVBoxLayout;
        auto keepWorkspaceDataCheckBox = new QCheckBox(tr("Keep Workspace data"));
        keepWorkspaceDataCheckBox->setChecked(true);
        layout->addWidget(keepWorkspaceDataCheckBox);
        auto workspaceInstruction = new QLabel(tr("Workspace data is not part of the DSPX standard and is editor-specific. Different editors may interpret the same fields differently, which can potentially lead to unexpected behavior or compatibility issues."));
        workspaceInstruction->setWordWrap(true);
        layout->addWidget(workspaceInstruction);
        layout->addStretch();
        auto buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        auto button = new QPushButton(tr("Import"));
        button->setDefault(true);
        connect(button, &QPushButton::clicked, &dlg, &QDialog::accept);
        buttonLayout->addWidget(button);
        layout->addLayout(buttonLayout);
        dlg.setLayout(layout);
        dlg.setModal(true);
        if (dlg.exec() != QDialog::Accepted) {
            return false;
        }
        qCDebug(lcDspxFileImporter) << "Keep Workspace data:" << keepWorkspaceDataCheckBox->isChecked();
        if (!keepWorkspaceDataCheckBox->isChecked()) {
            model.content.workspace.clear();
            for (auto &track : model.content.tracks) {
                track.workspace.clear();
                for (auto &clip : track.clips) {
                    clip->workspace.clear();
                    if (clip->type != QDspx::Clip::Singing) {
                        continue;
                    }
                    auto singingClip = clip.staticCast<QDspx::SingingClip>();
                    for (auto &note : singingClip->notes) {
                        note.workspace.clear();
                    }
                }
            }
        }
        return true;
    }

}
