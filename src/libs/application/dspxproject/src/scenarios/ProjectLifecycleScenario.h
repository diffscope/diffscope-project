// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_H

#include <memory>

#include <QByteArray>
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
    struct ProjectFileError;

    /** The recovery input format. */
    enum class ProjectRecoverySource {
        ProjectCopy, /**< A serialized project copy. */
        DocumentLog /**< A document snapshot and commit log directory. */
    };

    /**
     * @brief Coordinates project creation, opening, recovery, saving, and closing.
     *
     * Window parents are optional. Contexts returned as raw pointers are borrowed
     * from the strategy after presentation. Creation and opening return nullptr
     * on cancellation, failure, or context destruction during presentation.
     */
    class DSPXPROJECT_EXPORT ProjectLifecycleScenario : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(ProjectLifecycleScenario)
    public:
        /** The outcome of a save, replacement, or close preparation. */
        enum class Status {
            Success, /**< The operation completed. */
            Cancelled, /**< Interaction was cancelled or the context was destroyed. */
            Failed /**< A conflict or file operation error prevented completion. */
        };
        Q_ENUM(Status)

        /** The file association established by an in-place open. */
        enum class FileReplacement {
            Associate, /**< Retains the file association. */
            Import /**< Uses the file as a read-only source. */
        };

        /**
         * Creates a scenario without taking ownership of strategy.
         * @pre strategy is non-null and remains valid during scenario calls.
         */
        explicit ProjectLifecycleScenario(ProjectLifecycleStrategy *strategy, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~ProjectLifecycleScenario() override;

        /** Creates and presents a default project, using the untitled name if name is empty. */
        Q_INVOKABLE ProjectDocumentContext *newProject(const QString &name = {}, QWindow *parent = nullptr);
        /**
         * Creates a default project without presenting it, using the untitled name if name is empty.
         * Creates a non-file document if fileBacked is false. Passes externallyManaged to the document context.
         * Returns the owned context on success or nullptr on failure.
         */
        std::unique_ptr<ProjectDocumentContext> createUntitledProject(bool fileBacked, bool externallyManaged, const QString &name = {});
        /**
         * Creates and presents a project from a template, or returns nullptr on failure.
         * Requests a path if empty. Uses the untitled name if name is empty.
         */
        Q_INVOKABLE ProjectDocumentContext *newProjectFromTemplate(const QString &path, QWindow *parent = nullptr, const QString &name = {});
        /**
         * Opens and presents a project, or activates an existing context for its canonical path.
         * Requests a path if empty. Returns nullptr on cancellation, failure, or a concurrent open.
         */
        Q_INVOKABLE ProjectDocumentContext *openProject(const QString &path, QWindow *parent = nullptr);
        /** Dispatches paths through the strategy, requesting a single path if the list is empty. */
        Q_INVOKABLE void openFiles(const QStringList &paths, QWindow *parent = nullptr);

        /**
         * Restores and presents a project from a copy or document log directory.
         * Returns nullptr on failure. Reports partial log recovery
         * before retrying with an invalid commit log tail discarded.
         * @pre path is nonempty.
         */
        ProjectDocumentContext *recoverProject(ProjectRecoverySource source, const QString &path, const QString &name, QWindow *parent = nullptr);

        /**
         * Creates and presents a project from model, or returns nullptr on failure.
         * Uses the untitled name if name is empty. Creates a non-file document if
         * fileBacked is false.
         */
        ProjectDocumentContext *newProjectFromModel(const opendspx::Model &model, const QString &name = {}, QWindow *parent = nullptr, bool fileBacked = true);

        /**
         * Saves the project, requesting a destination for an unsaved file document.
         * Returns the save outcome. @pre context is non-null with an initialized document.
         */
        Q_INVOKABLE Status saveProject(ProjectDocumentContext *context, QWindow *parent = nullptr);
        /**
         * Saves the project to a new destination, requesting a path if empty.
         * Returns the save outcome. @pre context is non-null with an initialized document.
         */
        Q_INVOKABLE Status saveProjectAs(ProjectDocumentContext *context, const QString &path = {}, QWindow *parent = nullptr);
        /**
         * Saves a project copy, requesting a path if empty, without changing the current file.
         * Returns the save outcome. @pre context is non-null with an initialized document.
         */
        Q_INVOKABLE Status saveProjectCopy(ProjectDocumentContext *context, const QString &path = {}, QWindow *parent = nullptr);
        /**
         * Saves the associated file without prompting. Returns InvalidState if there is no
         * current path, NeedsConfirmation if the file was modified externally, or the save or conflict result.
         * @pre context is non-null.
         */
        ProjectFileError saveAssociatedFile(ProjectDocumentContext *context);
        /**
         * Requests a destination and saves the live project to its first associated file.
         * Returns InvalidState if the project already has a path, Cancelled if path selection ends without a path
         * or cancelOnDestroyed is destroyed, TooLarge if the UTF-8 path exceeds maximumPathBytes,
         * or the save result. A negative maximumPathBytes permits paths of any length.
         * Associates the path only on success. @pre context is non-null.
         */
        ProjectFileError saveProjectToFile(ProjectDocumentContext *context, qsizetype maximumPathBytes = -1,
                                          QWindow *parent = nullptr, QObject *cancelOnDestroyed = nullptr);
        /**
         * Loads path into context without prompting and adopts the file association on success.
         * Returns PathInUse if another project owns path, Busy if an operation conflicts,
         * or the validation and load result. @pre context is non-null.
         */
        ProjectFileError restoreProjectFile(ProjectDocumentContext *context, const QString &path);
        /**
         * Requests a source file and loads it into the existing context after confirmation.
         * Associate checks path ownership and the maximum UTF-8 path size. Import returns a
         * snapshot without a file association and checks its size before replacing the document.
         * Returns Cancelled if selection or confirmation is cancelled or cancelOnDestroyed is destroyed,
         * TooLarge if the resulting path or snapshot exceeds maximumBytes, or the load or conflict result.
         * @pre context has an initialized document and maximumBytes is nonnegative.
         */
        ProjectFileError replaceProjectFromFile(ProjectDocumentContext *context, FileReplacement replacement,
                                                QByteArray &snapshot, qsizetype maximumBytes,
                                                QWindow *parent = nullptr, QObject *cancelOnDestroyed = nullptr);
        /**
         * Saves a snapshot using the configured compression policy without prompting.
         * Returns Busy if an operation conflicts, or the snapshot save result. Updates data only on success.
         * @pre context is non-null.
         */
        ProjectFileError saveProjectSnapshot(ProjectDocumentContext *context, QByteArray &data);
        /**
         * Captures a snapshot and releases the file association while keeping the live document.
         * Returns InvalidState if the project has no file association, TooLarge if the snapshot exceeds
         * maximumBytes, or the snapshot save or conflict result. Updates data and releases the association
         * only on success. @pre context is non-null and maximumBytes is nonnegative.
         */
        ProjectFileError detachProjectFile(ProjectDocumentContext *context, QByteArray &data, qsizetype maximumBytes);
        /**
         * Restores a snapshot using the configured checker without prompting. Clears the file association on success.
         * Returns Busy if an operation conflicts, or the validation and load result. @pre context is non-null.
         */
        ProjectFileError restoreProjectSnapshot(ProjectDocumentContext *context, const QByteArray &data);
        /**
         * Resolves unsaved changes without destroying the context.
         * Returns Success if closing may proceed, Cancelled if declined, or Failed on error.
         * Discard writes a recovery copy. Non-file documents require no confirmation.
         * @pre context is non-null with an initialized document.
         */
        Q_INVOKABLE Status prepareClose(ProjectDocumentContext *context, QWindow *parent = nullptr);

    private:
        QScopedPointer<ProjectLifecycleScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTLIFECYCLESCENARIO_H
