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

    /** The recovery input format. */
    enum class ProjectRecoverySource {
        ProjectCopy, /**< A serialized project copy. */
        DocumentLog /**< A document snapshot and commit log directory. */
    };

    /**
     * @brief Coordinates project creation, opening, recovery, saving, and closing.
     *
     * Window parents are optional. Returned contexts are borrowed from the strategy
     * after presentation. Creation and opening return nullptr on cancellation,
     * failure, or context destruction during presentation.
     */
    class DSPXPROJECT_EXPORT ProjectLifecycleScenario : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(ProjectLifecycleScenario)
    public:
        /** The outcome of a save or close preparation. */
        enum class Status {
            Success, /**< The operation completed. */
            Cancelled, /**< Interaction was cancelled or the context was destroyed. */
            Failed /**< A conflict or file operation error prevented completion. */
        };
        Q_ENUM(Status)

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
