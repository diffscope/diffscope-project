// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_P_H

#include <dspxproject/ProjectLifecycleScenario.h>

#include <memory>

#include <QSet>

#include <dspxproject/private/ProjectDocumentContext_p.h>

namespace DspxProject {

    struct ProjectLifecycleOptions;

    class ProjectLifecycleScenarioPrivate {
    public:
        using Status = ProjectLifecycleScenario::Status;
        using SaveMode = ProjectDocumentContextPrivate::SaveMode;

        ProjectDocumentContext *openProject(QWindow *parent, QString path, bool asTemplate, const QString &name);
        ProjectDocumentContext *newProjectFromModel(QWindow *parent, const opendspx::Model &model, const QString &name, bool fileBacked, const QString &recoverySourceDirectory = {});
        ProjectDocumentContext *presentProject(QWindow *parent, const opendspx::Model &model, const QString &name, std::unique_ptr<Core::FileLocker> locker, const ProjectLifecycleOptions &options, const QString &sourcePath = {}, const QString &recoverySourceDirectory = {});
        Status saveProjectWithGuard(ProjectDocumentContext *context, QWindow *parent, SaveMode mode, const QString &path = {});
        ProjectFileError saveProject(ProjectDocumentContext *context, QWindow *parent, const ProjectLifecycleOptions &options,
                                     SaveMode mode, QString path = {}, bool overwriteConfirmed = false,
                                     qsizetype maximumPathBytes = -1, QObject *cancelOnDestroyed = nullptr);
        Status prepareClose(ProjectDocumentContext *context, QWindow *parent);
        ProjectFileError checkPath(const QString &path, ProjectDocumentContext *context, bool copy = false) const;

        Q_DECLARE_PUBLIC(ProjectLifecycleScenario)
        ProjectLifecycleScenario *q_ptr;
        ProjectLifecycleStrategy *strategy;
        QSet<QString> fileOperationPaths;
        QSet<ProjectDocumentContext *> busyProjects;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_P_H
