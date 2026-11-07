// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <dspxproject/dspxprojectglobal.h>

namespace opendspx {
    struct Model;
}

namespace Core {
    class FileLocker;
}

namespace DspxProject {

    class Document;
    class IChecker;
    class ProjectDocumentContextPrivate;

    /** The result of a project file operation. */
    struct ProjectFileError {
        /** The operation outcome. */
        enum Code {
            None, /**< The operation succeeded. */
            Read, /**< The source could not be read. */
            Parse, /**< The source could not be deserialized. */
            Load, /**< The model could not be loaded. */
            Write, /**< Serialization or destination writing failed. */
            Restore, /**< Recovery data could not be restored. */
            Busy, /**< A conflicting operation prevents access. */
            InvalidState, /**< The context state or destination is unsuitable. */
            CopyToCurrentFile, /**< A copy targets the current project file. */
            CheckerWarning, /**< Validation warnings require acceptance. */
            IntegrityWarning, /**< Serialization integrity warnings require acceptance. */
        };

        /** The result code, None on success. */
        Code code = None;
        /** The affected path, empty if no path applies. */
        QString path;
        /** The diagnostic detail, empty if unavailable. */
        QString detail;
    };

    /**
     * @brief Owns an initialized project document and its optional file association.
     *
     * Construction creates an uninitialized context. A successful openFile() or
     * newFile() initializes the document exactly once. File operations return
     * ProjectFileError::None on success and an error code otherwise.
     * Recovery logging is enabled if Options::recoveryRootPath is nonempty.
     */
    class DSPXPROJECT_EXPORT ProjectDocumentContext : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(ProjectDocumentContext)
        Q_PROPERTY(Core::FileLocker *fileLocker READ fileLocker CONSTANT)
        Q_PROPERTY(Document *document READ document CONSTANT)
        Q_PROPERTY(QString defaultDocumentName READ defaultDocumentName NOTIFY defaultDocumentNameChanged)
    public:
        /** The editor identity and document recovery configuration. */
        struct Options {
            /** The editor identifier written during initialization and saving. */
            QString editorId;
            /** The editor display name written during initialization and saving. */
            QString editorName;
            /** The editor version recorded in the project workspace. */
            QString editorVersion;
            /** The fallback recovery display name. */
            QString untitledName;
            /** The recovery root directory, empty to disable document logging. */
            QString recoveryRootPath;
        };

        /** The source reading and validation policy. */
        struct OpenOptions {
            /** The borrowed checker for this operation, or nullptr to skip validation. */
            IChecker *checker = nullptr;
            /** Whether an existing project file remains open and locked after reading. */
            bool keepFileOpen = false;
            /** Whether detected checker warnings permit initialization. */
            bool acceptCheckerWarnings = false;
        };

        /** The serialization policy. */
        struct SaveOptions {
            /** Whether to compress the serialized project. */
            bool compress = true;
            /** Whether serialization integrity warnings permit writing. */
            bool acceptIntegrityWarnings = false;
        };

        /** Creates an uninitialized context with the specified configuration. */
        explicit ProjectDocumentContext(const Options &options, QObject *parent = nullptr);
        /** Destroys the document, releases file locks, and removes active recovery data. */
        ~ProjectDocumentContext() override;

        /**
         * Returns the borrowed file locker, or nullptr before initialization or for a
         * non-file document. A new file document has a locker with an empty path.
         */
        Core::FileLocker *fileLocker() const;

        /** Returns the owned document, or nullptr before successful initialization. */
        Document *document() const;

        /** Returns the suggested document name, which may be empty. */
        QString defaultDocumentName() const;
        /** Sets the suggested document name and updates the recovery display name. */
        void setDefaultDocumentName(const QString &name);

        /**
         * Opens a project file and initializes this context.
         * Returns InvalidState if already initialized, or the reading, validation,
         * or initialization result otherwise.
         * @pre path is nonempty.
         */
        ProjectFileError openFile(const QString &path, const OpenOptions &options);
        /**
         * Initializes a new document from model with the suggested name.
         * Creates no file locker if isNonFileDocument is true. Returns InvalidState
         * if already initialized, or the initialization result otherwise.
         */
        ProjectFileError newFile(const opendspx::Model &model, const QString &name, bool isNonFileDocument);
        /**
         * Initializes a new document from a template without associating its source path.
         * Creates no file locker if isNonFileDocument is true. Returns InvalidState
         * if already initialized, or the reading, validation, or initialization result.
         * @pre templatePath is nonempty.
         */
        ProjectFileError newFile(const QString &templatePath, const QString &name, bool isNonFileDocument, const OpenOptions &options);
        /**
         * Saves to the current path and marks the transaction history clean on success.
         * Returns InvalidState without an initialized file document and a current path,
         * or the serialization and writing result otherwise.
         */
        ProjectFileError save(const SaveOptions &options);
        /**
         * Saves a file document to path and adopts that file association on success.
         * Marks the history clean and clears the suggested name on success. Returns
         * InvalidState without an initialized file document, or the serialization
         * and writing result otherwise. @pre path is nonempty.
         */
        ProjectFileError saveAs(const QString &path, const SaveOptions &options);
        /**
         * Saves a copy without changing the file association or clean history position.
         * Supports non-file documents. Returns InvalidState before initialization,
         * CopyToCurrentFile for the current file, or the save result. @pre path is nonempty.
         */
        ProjectFileError saveCopy(const QString &path, const SaveOptions &options);

    Q_SIGNALS:
        /** Notifies that save() or saveAs() succeeded. */
        void saved();
        /** Notifies that the suggested document name changed. */
        void defaultDocumentNameChanged();

    private:
        QScopedPointer<ProjectDocumentContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H
