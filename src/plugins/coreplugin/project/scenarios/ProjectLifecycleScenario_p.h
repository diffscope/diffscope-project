// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLESCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLESCENARIO_P_H

#include <coreplugin/ProjectLifecycleScenario.h>

#include <memory>

#include <QSet>

#include <coreplugin/private/ProjectDocumentContext_p.h>

namespace Core {

    struct ProjectLifecycleOptions;

    class ProjectLifecycleScenarioPrivate {
    public:
        using Status = ProjectLifecycleScenario::Status;
        using SaveMode = ProjectDocumentContextPrivate::SaveMode;

        ProjectDocumentContext *openProject(QWindow *parent, QString path, bool asTemplate, const QString &name);
        ProjectDocumentContext *presentProject(QWindow *parent, const opendspx::Model &model, const QString &name, std::unique_ptr<FileLocker> locker, const ProjectLifecycleOptions &options, const QString &sourcePath = {});
        Status saveProjectWithGuard(ProjectDocumentContext *context, QWindow *parent, SaveMode mode, const QString &path = {});
        Status saveProject(ProjectDocumentContext *context, QWindow *parent, const ProjectLifecycleOptions &options, SaveMode mode, QString path = {});
        Status prepareClose(ProjectDocumentContext *context, QWindow *parent);

        Q_DECLARE_PUBLIC(ProjectLifecycleScenario)
        ProjectLifecycleScenario *q_ptr;
        ProjectLifecycleStrategy *strategy;
        QSet<QString> openingPaths;
        QSet<ProjectDocumentContext *> busyProjects;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLESCENARIO_P_H
