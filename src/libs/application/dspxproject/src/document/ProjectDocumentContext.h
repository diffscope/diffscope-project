// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H

#include <QByteArray>
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
    class IDocumentPersistenceHook;
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
            Cancelled, /**< The operation was cancelled. */
            PathInUse, /**< The path is associated with a different project. */
            NeedsConfirmation, /**< A file change or overwrite requires confirmation. */
            InvalidState, /**< The context state or destination is unsuitable. */
            CopyToCurrentFile, /**< A copy targets the current project file. */
            CheckerWarning, /**< Validation warnings require acceptance. */
            IntegrityWarning, /**< Serialization integrity warnings require acceptance. */
            TooLarge, /**< The serialized snapshot or path exceeds the requested size limit. */
        };

        /** The result code, None on success. */
        Code code = None;
        /** The affected path, empty if no path applies. */
        QString path;
        /** The diagnostic detail, empty if unavailable. */
        QString detail;
        /** The warning title if code is CheckerWarning. */
        QString message;
    };

    /**
     * @brief Owns an initialized project document and its optional file association.
     *
     * Construction creates an uninitialized context. Once initialized, the context
     * keeps the same document object when reloading a file or loading a snapshot.
     * Persistence operations return ProjectFileError::None on success and an error code otherwise.
     * Recovery logging is enabled if Options::recoveryRootPath is nonempty.
     */
    class DSPXPROJECT_EXPORT ProjectDocumentContext : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(ProjectDocumentContext)
        Q_PROPERTY(Core::FileLocker *fileLocker READ fileLocker NOTIFY fileLockerChanged)
        Q_PROPERTY(Document *document READ document CONSTANT)
        Q_PROPERTY(QString defaultDocumentName READ defaultDocumentName NOTIFY defaultDocumentNameChanged)
        Q_PROPERTY(bool fileOperationsAllowed READ fileOperationsAllowed NOTIFY fileLockerChanged)
    public:
        /** The document persistence configuration. */
        struct Options {
            /** The fallback recovery display name. */
            QString untitledName;
            /** The recovery root directory, empty to disable document logging. */
            QString recoveryRootPath;
            /** The borrowed persistence hook, valid for the context's lifetime if non-null. */
            IDocumentPersistenceHook *persistenceHook = nullptr;
            /** Whether to save the current file atomically and disable file saves for non-file documents. */
            bool externallyManaged = false;
            /** Whether saving attempts to keep the file open. Applies to ordinary file documents. */
            bool keepFileOpenAfterSave = false;
        };

        /** The source reading and validation policy. */
        struct OpenOptions {
            /** The borrowed checker for this operation, or nullptr to skip validation. */
            IChecker *checker = nullptr;
            /** Whether an existing project file remains open and locked after reading. */
            bool keepFileOpen = false;
            /** Whether detected checker warnings permit the load to proceed. */
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
        /** Destroys the document, releases file locks, and removes active recovery data unless preservation was requested. */
        ~ProjectDocumentContext() override;

        /**
         * Returns the borrowed file locker, or nullptr before initialization or for a
         * non-file document. A new file document has a locker with an empty path.
         * Reacquire the locker after fileLockerChanged().
         */
        Core::FileLocker *fileLocker() const;
        /** Returns the source or former project file path retained for relative references, or an empty string if absent. */
        QString retainedReferenceFilePath() const;
        /** Returns whether file operations are enabled for the persistence mode. Externally managed contexts require a file locker. */
        bool fileOperationsAllowed() const;
        /** Returns whether two nonempty paths refer to the same file or resolve to the same destination. */
        static bool sameFilePath(const QString &first, const QString &second);

        /** Returns the owned document, or nullptr before successful initialization. */
        Document *document() const;

        /**
         * Returns the active document log directory path, or an empty string if logging is unavailable.
         */
        QString documentLogDirectory() const;

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
         * Loads path and adopts its file association on success. Replaces an existing
         * document's contents and resets its undo history while preserving the document object.
         * Returns the read, validation, or load result. @pre path is nonempty.
         */
        ProjectFileError reloadFile(const QString &path, const OpenOptions &options);
        /**
         * Loads an OpenDSPX snapshot as a non-file document. Replaces an existing
         * document's contents and releases its file association on success.
         * Returns the decoding, validation, or load result.
         */
        ProjectFileError loadSnapshot(const QByteArray &data, const OpenOptions &options);
        /**
         * Serializes an OpenDSPX snapshot into data without changing the file association or clean history position.
         * Returns InvalidState before initialization, Busy if access is unavailable,
         * IntegrityWarning if the options reject a warning, or the serialization result. Updates data only on success.
         */
        ProjectFileError saveSnapshot(QByteArray &data, const SaveOptions &options);
        /** Retains the active recovery data when the context is destroyed. */
        void preserveRecoveryLog();
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
         * Uses save() if path refers to the current file. Marks the history clean on
         * success and clears the suggested name if adopting a new path. Returns
         * InvalidState without an initialized file document, or the serialization
         * and writing result otherwise. @pre path is nonempty.
         */
        ProjectFileError saveAs(const QString &path, const SaveOptions &options);
        /**
         * Saves a copy without changing the file association or clean history position.
         * Returns InvalidState before initialization or for an externally managed non-file
         * document, CopyToCurrentFile if path refers to the current file, or the save result.
         * @pre path is nonempty.
         */
        ProjectFileError saveCopy(const QString &path, const SaveOptions &options);
        /**
         * Saves a copy to model without changing the file association or clean history position.
         * Supports non-file documents. Returns InvalidState before initialization, Busy if
         * access is unavailable, Write if model preparation fails, or None on success.
         * Updates model only on success.
         */
        ProjectFileError saveCopy(opendspx::Model &model);

    Q_SIGNALS:
        /** Notifies that the file locker was replaced, created, or removed. */
        void fileLockerChanged();
        /** Notifies that an existing document is about to be replaced after preparing the candidate. */
        void aboutToReload();
        /** Notifies that the replacement and its file association have been committed. */
        void reloaded();
        /** Notifies that save() or saveAs() succeeded. */
        void saved();
        /** Notifies that the suggested document name changed. */
        void defaultDocumentNameChanged();

    private:
        QScopedPointer<ProjectDocumentContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H
