// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREPROJECTLIFECYCLESTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREPROJECTLIFECYCLESTRATEGY_H

#include <QCoreApplication>

#include <coreplugin/ProjectLifecycleStrategy.h>

namespace Core::Internal {

    class CoreProjectLifecycleStrategy : public ProjectLifecycleStrategy {
        Q_DECLARE_TR_FUNCTIONS(Core::Internal::CoreProjectLifecycleStrategy)
    public:
        // The checker must be non-null and remain valid during strategy calls.
        explicit CoreProjectLifecycleStrategy(IDspxChecker *checker);

        static QString recoveryRootPath();
        static QString recoveryCopyPath();

        ProjectLifecycleOptions projectLifecycleOptions() const override;
        ProjectDocumentContext *findOpenProject(const QString &canonicalPath) const override;
        void activateProject(ProjectDocumentContext *context) override;
        void presentProject(std::unique_ptr<ProjectDocumentContext> context) override;
        void recordRecentFile(ProjectDocumentContext *context, bool captureThumbnail) override;
        void dispatchFiles(const QStringList &paths, QWindow *parent) override;

        QString requestPath(const ProjectPathRequest &request, QWindow *parent) override;
        bool confirmWarning(const ProjectWarning &warning, QWindow *parent) override;
        ExternalChangeChoice resolveExternalChange(const QString &path, QWindow *parent) override;
        CloseChoice confirmClose(const QString &projectName, const QString &recoveryPath, QWindow *parent) override;
        void notifyPartialRecovery(const ProjectFileError &error, QWindow *parent) override;
        void reportError(const ProjectFileError &error, QWindow *parent) override;

    private:
        IDspxChecker *m_checker;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREPROJECTLIFECYCLESTRATEGY_H
