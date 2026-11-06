// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <qqmlintegration.h>

#include <dspxproject/dspxprojectglobal.h>

class QWindow;

namespace opendspx {
    struct Model;
}

namespace DspxProject {

    class ProjectDocumentContext;
    class ProjectLifecycleScenarioPrivate;
    class ProjectLifecycleStrategy;

    enum class ProjectRecoverySource { ProjectCopy, DocumentLog };

    class DSPXPROJECT_EXPORT ProjectLifecycleScenario : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(ProjectLifecycleScenario)
    public:
        enum class Status { Success, Cancelled, Failed };
        Q_ENUM(Status)

        // The strategy must be non-null and remain valid during scenario calls.
        explicit ProjectLifecycleScenario(ProjectLifecycleStrategy *strategy, QObject *parent = nullptr);
        ~ProjectLifecycleScenario() override;

        Q_INVOKABLE ProjectDocumentContext *newProject(const QString &name = {}, QWindow *parent = nullptr);
        Q_INVOKABLE ProjectDocumentContext *newProjectFromTemplate(const QString &path, QWindow *parent = nullptr, const QString &name = {});
        Q_INVOKABLE ProjectDocumentContext *openProject(const QString &path, QWindow *parent = nullptr);
        Q_INVOKABLE void openFiles(const QStringList &paths, QWindow *parent = nullptr);

        ProjectDocumentContext *recoverProject(ProjectRecoverySource source, const QString &path, const QString &name, QWindow *parent = nullptr);

        ProjectDocumentContext *newProjectFromModel(const opendspx::Model &model, const QString &name = {}, QWindow *parent = nullptr, bool fileBacked = true);

        // The context must be non-null and contain an initialized document.
        Q_INVOKABLE Status saveProject(ProjectDocumentContext *context, QWindow *parent = nullptr);
        Q_INVOKABLE Status saveProjectAs(ProjectDocumentContext *context, const QString &path = {}, QWindow *parent = nullptr);
        Q_INVOKABLE Status saveProjectCopy(ProjectDocumentContext *context, const QString &path = {}, QWindow *parent = nullptr);
        Q_INVOKABLE Status prepareClose(ProjectDocumentContext *context, QWindow *parent = nullptr);

    private:
        QScopedPointer<ProjectLifecycleScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_H
