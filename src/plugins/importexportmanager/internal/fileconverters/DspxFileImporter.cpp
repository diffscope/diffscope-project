// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DspxFileImporter.h"

#include <memory>
#include <sstream>

#include <QCheckBox>
#include <QDialog>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow>

#include <opendspx/model.h>
#include <opendspx/serializer/serializer.h>
#include <opendspx/singingclip.h>

#include <coreplugin/OpenSaveProjectFileScenario.h>

#include <importexportmanager/internal/DsEditorLiteParameterMapper.h>

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

    bool DspxFileImporter::execImport(const QString &path, opendspx::Model &model, QWindow *window) {
        Core::OpenSaveProjectFileScenario scenario;
        scenario.setWindow(window);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCCritical(lcDspxFileImporter) << "Failed to read file:" << path << file.errorString();
            scenario.showOpenFailMessageBox(path, file.errorString());
            return false;
        }
        opendspx::SerializationErrorList errors;
        std::stringstream in(file.readAll().toStdString(), std::ios::in);
        model = opendspx::Serializer::deserialize(in, errors);
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
        auto mapDsEditorLiteParametersCheckBox = new QCheckBox(tr("Map DS Editor Lite DiffSinger parameters"));
        mapDsEditorLiteParametersCheckBox->setChecked(false);
        layout->addWidget(mapDsEditorLiteParametersCheckBox);
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
        qCDebug(lcDspxFileImporter) << "Map DS Editor Lite DiffSinger parameters:" << mapDsEditorLiteParametersCheckBox->isChecked();
        if (mapDsEditorLiteParametersCheckBox->isChecked()) {
            DsEditorLiteParameterMapper::mapModel(model, DsEditorLiteParameterMapper::Direction::FromDsEditorLite);
        }
        qCDebug(lcDspxFileImporter) << "Keep Workspace data:" << keepWorkspaceDataCheckBox->isChecked();
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
        return true;
    }

}
