// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEMMANAGER_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEMMANAGER_H

#include <functional>
#include <optional>

#include <QList>
#include <QObject>
#include <QScopedPointer>

#include <dspxmodelORM/Handle.h>

#include <dspxproject/DocumentProblem.h>
#include <dspxproject/dspxprojectglobal.h>

namespace DspxProject {

    class Document;
    class DocumentProblemManagerPrivate;

    /**
     * Manages runtime diagnostics on the document thread.
     * Publication, queries, and operation dispatch require a settled model.
     */
    class DSPXPROJECT_EXPORT DocumentProblemManager : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DocumentProblemManager)
    public:
        ~DocumentProblemManager() override;

        /** Returns whether the operation applies, without side effects. */
        using Predicate = std::function<bool(const DocumentProblem &)>;
        /** Returns whether the operation handled the request. */
        using Handler = std::function<bool(const DocumentProblem &)>;

        /**
         * Replaces the owner's diagnostics and clears them on owner destruction.
         * Returns false for invalid input without replacing the batch.
         */
        bool replaceProblems(QObject *owner, const QList<DocumentProblemReport> &reports);

        /** Returns matching diagnostics in ID order after model changes have settled. */
        QList<DocumentProblem> problems(const dspx::EntityObject *entity, const DocumentProblemFilter &filter = {}) const;
        /** Returns the current visible diagnostic, or nullopt if unavailable. */
        std::optional<DocumentProblem> problem(DocumentProblemId id) const;
        /** Returns the target and its ancestors in order, or an empty list if unavailable. */
        QList<dspx::EntityObject *> entityPath(DocumentProblemId id) const;

        /** Registers a navigation observer and returns its ID, or Invalid for invalid input. */
        DocumentProblemOperationId registerNavigationHandler(QObject *context, Handler handler, Predicate applies = {});
        /** Registers an action for type and returns its ID, or Invalid for invalid input. */
        DocumentProblemOperationId registerAction(QObject *context, const QString &type, const QString &text,
                                                  Handler handler, Predicate applies = {});
        /** Removes the registration and returns whether it existed. */
        bool unregisterOperation(DocumentProblemOperationId operationId);

        /** Returns whether an applicable navigation observer is registered. */
        bool canNavigate(DocumentProblemId id) const;
        /** Calls applicable observers in registration order and returns whether any handled the request. */
        bool navigate(DocumentProblemId id);
        /** Returns applicable actions in registration order. */
        QList<DocumentProblemAction> actions(DocumentProblemId id) const;
        /** Calls the selected action if available and returns whether it handled the request. */
        bool triggerAction(DocumentProblemId id, DocumentProblemOperationId actionId);

    Q_SIGNALS:
        /** Notifies that queries for the listed entities may have changed. */
        void problemsChanged(const QList<dspx::Handle> &affectedEntities);
        /** Notifies that operation registrations changed. */
        void operationsChanged();

    private:
        friend class Document;
        explicit DocumentProblemManager(Document *document);

        QScopedPointer<DocumentProblemManagerPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEMMANAGER_H
