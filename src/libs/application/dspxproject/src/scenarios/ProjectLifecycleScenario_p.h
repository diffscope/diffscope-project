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
        ProjectDocumentContext *presentProject(QWindow *parent, const opendspx::Model &model, const QString &name, std::unique_ptr<Core::FileLocker> locker, const ProjectLifecycleOptions &options, const QString &sourcePath = {});
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

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_P_H
