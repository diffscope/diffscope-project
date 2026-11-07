// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESTRATEGY_H

#include <memory>

#include <QObject>
#include <QStringList>

#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/dspxprojectglobal.h>

class QWindow;

namespace DspxProject {

    class IChecker;

    /** The editor defaults and project file policies for a lifecycle operation. */
    struct ProjectLifecycleOptions {
        /** The document context configuration. */
        ProjectDocumentContext::Options document;
        /** The initial track name in a default project. */
        QString defaultTrackName;
        /** The initial singing clip name in a default project. */
        QString defaultClipName;
        /** The destination for the recovery copy created when changes are discarded. */
        QString recoveryCopyPath;
        /** The borrowed required checker. @pre Non-null and valid for the operation. */
        IChecker *checker = nullptr;
        /** Whether an opened project file remains locked after reading. */
        bool keepOpenedFileLocked = false;
        /** Whether saving detects external file modifications. */
        bool checkExternalChanges = true;
        /** Whether project serialization uses compression. */
        bool compress = true;
    };

    /** The context for a project path selection. */
    struct ProjectPathRequest {
        /** The file operation requiring a path. */
        enum Purpose {
            Open, /**< Opening a project. */
            Template, /**< Creating a project from a template. */
            SaveAs, /**< Changing the project destination. */
            SaveCopy /**< Writing an independent project copy. */
        };

        /** The requested file operation. */
        Purpose purpose;
        /** The current project path, empty for an unsaved document. */
        QString currentPath;
        /** The suggested document name, empty if unavailable. */
        QString suggestedName;
    };

    /** A lifecycle warning requiring a decision before continuation. */
    struct ProjectWarning {
        /** The warning category. */
        enum Code {
            OtherEditor, /**< The source identifies a different editor. */
            IncompatibleVersion, /**< The source editor version is incompatible. */
            CheckerWarning, /**< Project validation reported warnings. */
            SaveIntegrity /**< Serialization reported integrity warnings. */
        };

        /** The warning category for this operation. */
        Code code;
        /** The affected project path. */
        QString path;
        /** The editor name, version, or checker message, empty if unavailable. */
        QString detail;
    };

    /**
     * @brief Supplies application services and user interaction for project lifecycle operations.
     *
     * Window parents are optional. Context arguments refer to initialized documents
     * and must be non-null. Context ownership remains with the caller except for
     * presentProject(), which transfers ownership to the implementation.
     */
    class DSPXPROJECT_EXPORT ProjectLifecycleStrategy : public QObject {
        Q_OBJECT
    public:
        /** The resolution of an external project file modification. */
        enum class ExternalChangeChoice {
            SaveAs, /**< Select a different destination. */
            Overwrite, /**< Replace the externally modified file. */
            Cancel /**< Cancel saving. */
        };
        /** The resolution of unsaved changes during closing. */
        enum class CloseChoice {
            Save, /**< Save the current project. */
            Discard, /**< Write a recovery copy and permit closing. */
            Cancel /**< Cancel closing. */
        };

        /** Creates a lifecycle strategy with an optional QObject parent. */
        explicit ProjectLifecycleStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~ProjectLifecycleStrategy() override;

        /**
         * Returns the configuration for the operation.
         * @post The returned checker is non-null and remains valid for the operation.
         */
        virtual ProjectLifecycleOptions projectLifecycleOptions() const = 0;
        /** Returns the borrowed open context for canonicalPath, or nullptr if absent. */
        virtual ProjectDocumentContext *findOpenProject(const QString &canonicalPath) const = 0;

        /** Activates the existing initialized project context. */
        virtual void activateProject(ProjectDocumentContext *context) = 0;
        /** Presents an initialized project and takes ownership of context. */
        virtual void presentProject(std::unique_ptr<ProjectDocumentContext> context) = 0;
        /** Records the project in recent files, optionally capturing a thumbnail. */
        virtual void recordRecentFile(ProjectDocumentContext *context, bool captureThumbnail) = 0;
        /** Dispatches file paths to the application's file handlers. */
        virtual void dispatchFiles(const QStringList &paths, QWindow *parent) = 0;

        /** Returns a selected path, or an empty string on cancellation. */
        virtual QString requestPath(const ProjectPathRequest &request, QWindow *parent) = 0;
        /** Returns whether the user accepts the warning and permits continuation. */
        virtual bool confirmWarning(const ProjectWarning &warning, QWindow *parent) = 0;
        /** Returns the user's resolution of an externally modified project file. */
        virtual ExternalChangeChoice resolveExternalChange(const QString &path, QWindow *parent) = 0;
        /** Returns the user's close decision with recoveryPath as the discard copy destination. */
        virtual CloseChoice confirmClose(const QString &projectName, const QString &recoveryPath, QWindow *parent) = 0;
        /** Reports a complete recovery failure before an attempt at partial recovery. */
        virtual void notifyPartialRecovery(const ProjectFileError &error, QWindow *parent) = 0;
        /** Reports a project file operation failure. */
        virtual void reportError(const ProjectFileError &error, QWindow *parent) = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESTRATEGY_H
