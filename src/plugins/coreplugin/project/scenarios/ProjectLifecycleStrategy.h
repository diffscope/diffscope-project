// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLESTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLESTRATEGY_H

#include <memory>

#include <QObject>
#include <QStringList>

#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/coreglobal.h>

class QWindow;

namespace Core {

    class IDspxChecker;

    struct ProjectLifecycleOptions {
        ProjectDocumentContext::Options document;
        QString defaultTrackName;
        QString defaultClipName;
        QString recoveryCopyPath;
        IDspxChecker *checker = nullptr;
        bool keepOpenedFileLocked = false;
        bool checkExternalChanges = true;
        bool compress = true;
    };

    struct ProjectPathRequest {
        enum Purpose { Open, Template, SaveAs, SaveCopy };

        Purpose purpose;
        QString currentPath;
        QString suggestedName;
    };

    struct ProjectWarning {
        enum Code { OtherEditor, IncompatibleVersion, CheckerWarning, SaveIntegrity };

        Code code;
        QString path;
        QString detail;
    };

    class CORE_EXPORT ProjectLifecycleStrategy : public QObject {
        Q_OBJECT
    public:
        enum class ExternalChangeChoice { SaveAs, Overwrite, Cancel };
        enum class CloseChoice { Save, Discard, Cancel };

        explicit ProjectLifecycleStrategy(QObject *parent = nullptr);
        ~ProjectLifecycleStrategy() override;

        // The options must provide a non-null checker that remains valid for the operation.
        virtual ProjectLifecycleOptions projectLifecycleOptions() const = 0;
        virtual ProjectDocumentContext *findOpenProject(const QString &canonicalPath) const = 0;

        // Project context arguments must be non-null.
        virtual void activateProject(ProjectDocumentContext *context) = 0;
        virtual void presentProject(std::unique_ptr<ProjectDocumentContext> context) = 0;
        virtual void recordRecentFile(ProjectDocumentContext *context, bool captureThumbnail) = 0;
        virtual void dispatchFiles(const QStringList &paths, QWindow *parent) = 0;

        virtual QString requestPath(const ProjectPathRequest &request, QWindow *parent) = 0;
        virtual bool confirmWarning(const ProjectWarning &warning, QWindow *parent) = 0;
        virtual ExternalChangeChoice resolveExternalChange(const QString &path, QWindow *parent) = 0;
        virtual CloseChoice confirmClose(const QString &projectName, const QString &recoveryPath, QWindow *parent) = 0;
        virtual void notifyPartialRecovery(const ProjectFileError &error, QWindow *parent) = 0;
        virtual void reportError(const ProjectFileError &error, QWindow *parent) = 0;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLESTRATEGY_H
