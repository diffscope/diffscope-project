// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREPROJECTLIFECYCLESTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREPROJECTLIFECYCLESTRATEGY_H

#include <QCoreApplication>

#include <dspxproject/ProjectLifecycleStrategy.h>

namespace Core::Internal {

    class CoreProjectLifecycleStrategy : public DspxProject::ProjectLifecycleStrategy {
        Q_DECLARE_TR_FUNCTIONS(Core::Internal::CoreProjectLifecycleStrategy)
    public:
        // The checker must be non-null and remain valid during strategy calls.
        explicit CoreProjectLifecycleStrategy(DspxProject::IChecker *checker);

        static QString recoveryRootPath();
        static QString recoveryCopyPath();

        DspxProject::ProjectLifecycleOptions projectLifecycleOptions() const override;
        DspxProject::ProjectDocumentContext *findOpenProject(const QString &canonicalPath) const override;
        void activateProject(DspxProject::ProjectDocumentContext *context) override;
        void presentProject(std::unique_ptr<DspxProject::ProjectDocumentContext> context) override;
        void recordRecentFile(DspxProject::ProjectDocumentContext *context, bool captureThumbnail) override;
        void dispatchFiles(const QStringList &paths, QWindow *parent) override;

        QString requestPath(const DspxProject::ProjectPathRequest &request, QWindow *parent) override;
        bool confirmWarning(const DspxProject::ProjectWarning &warning, QWindow *parent) override;
        ExternalChangeChoice resolveExternalChange(const QString &path, QWindow *parent) override;
        CloseChoice confirmClose(const QString &projectName, const QString &recoveryPath, QWindow *parent) override;
        void notifyPartialRecovery(const DspxProject::ProjectFileError &error, QWindow *parent) override;
        void reportError(const DspxProject::ProjectFileError &error, QWindow *parent) override;

    private:
        DspxProject::IChecker *m_checker;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREPROJECTLIFECYCLESTRATEGY_H
