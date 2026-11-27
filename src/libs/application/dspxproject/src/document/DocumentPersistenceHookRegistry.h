// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENTPERSISTENCEHOOKREGISTRY_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENTPERSISTENCEHOOKREGISTRY_H

#include <QList>
#include <QScopedPointer>

#include <dspxproject/IDocumentPersistenceHook.h>
#include <dspxproject/dspxprojectglobal.h>

namespace DspxProject {

    class DocumentPersistenceHookRegistryPrivate;

    /** Runs borrowed persistence hooks in registration order. */
    class DSPXPROJECT_EXPORT DocumentPersistenceHookRegistry : public IDocumentPersistenceHook {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DocumentPersistenceHookRegistry)
    public:
        explicit DocumentPersistenceHookRegistry(QObject *parent = nullptr);
        ~DocumentPersistenceHookRegistry() override;

        /**
         * Appends a hook without taking ownership.
         * @pre hook is non-null and remains valid for the registry's lifetime.
         */
        void registerHook(IDocumentPersistenceHook *hook);
        /** Returns the borrowed hooks in registration order. */
        QList<IDocumentPersistenceHook *> hooks() const;

        void beforeLoadModel(ProjectDocumentContext &context, opendspx::Model &model, const QString &recoverySourceDirectory) override;
        void afterSaveModel(ProjectDocumentContext &context, opendspx::Model &model, const QString &destination) override;
        void prepareReplacement(ProjectDocumentContext &context, ProjectDocumentContext &candidate) override;
        void finishReplacement(ProjectDocumentContext &context, ProjectDocumentContext &candidate) override;

    private:
        QScopedPointer<DocumentPersistenceHookRegistryPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENTPERSISTENCEHOOKREGISTRY_H
