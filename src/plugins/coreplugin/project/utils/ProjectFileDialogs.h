// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTFILEDIALOGS_H
#define DIFFSCOPE_COREPLUGIN_PROJECTFILEDIALOGS_H

#include <QCoreApplication>
#include <QScopedPointer>

#include <dspxproject/ProjectLifecycleStrategy.h>
#include <coreplugin/coreglobal.h>

class QWindow;

namespace Core {

    class ProjectFileDialogsPrivate;

    class CORE_EXPORT ProjectFileDialogs {
        Q_DECLARE_TR_FUNCTIONS(Core::ProjectFileDialogs)
        Q_DECLARE_PRIVATE(ProjectFileDialogs)
    public:
        explicit ProjectFileDialogs(QWindow *window = nullptr);
        ~ProjectFileDialogs();
        Q_DISABLE_COPY_MOVE(ProjectFileDialogs)

        QString requestPath(const DspxProject::ProjectPathRequest &request);
        bool confirmWarning(const DspxProject::ProjectWarning &warning);
        DspxProject::ProjectLifecycleStrategy::ExternalChangeChoice resolveExternalChange(const QString &path);
        DspxProject::ProjectLifecycleStrategy::CloseChoice confirmClose(const QString &projectName, const QString &recoveryPath);
        void notifyPartialRecovery(const DspxProject::ProjectFileError &error);
        void reportError(const DspxProject::ProjectFileError &error);

    private:
        QScopedPointer<ProjectFileDialogsPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTFILEDIALOGS_H
