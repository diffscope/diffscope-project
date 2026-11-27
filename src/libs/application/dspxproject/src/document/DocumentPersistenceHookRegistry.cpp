// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DocumentPersistenceHookRegistry.h"
#include "DocumentPersistenceHookRegistry_p.h"

namespace DspxProject {

    DocumentPersistenceHookRegistry::DocumentPersistenceHookRegistry(QObject *parent) : IDocumentPersistenceHook(parent), d_ptr(new DocumentPersistenceHookRegistryPrivate) {
        Q_D(DocumentPersistenceHookRegistry);
        d->q_ptr = this;
    }

    DocumentPersistenceHookRegistry::~DocumentPersistenceHookRegistry() = default;

    void DocumentPersistenceHookRegistry::registerHook(IDocumentPersistenceHook *hook) {
        Q_D(DocumentPersistenceHookRegistry);
        d->hooks.append(hook);
    }

    QList<IDocumentPersistenceHook *> DocumentPersistenceHookRegistry::hooks() const {
        Q_D(const DocumentPersistenceHookRegistry);
        return d->hooks;
    }

    void DocumentPersistenceHookRegistry::beforeLoadModel(ProjectDocumentContext &context, opendspx::Model &model, const QString &recoverySourceDirectory) {
        for (auto *hook : hooks()) {
            hook->beforeLoadModel(context, model, recoverySourceDirectory);
        }
    }

    void DocumentPersistenceHookRegistry::afterSaveModel(ProjectDocumentContext &context, opendspx::Model &model, const QString &destination) {
        for (auto *hook : hooks()) {
            hook->afterSaveModel(context, model, destination);
        }
    }

    void DocumentPersistenceHookRegistry::prepareReplacement(ProjectDocumentContext &context, ProjectDocumentContext &candidate) {
        for (auto *hook : hooks())
            hook->prepareReplacement(context, candidate);
    }

    void DocumentPersistenceHookRegistry::finishReplacement(ProjectDocumentContext &context, ProjectDocumentContext &candidate) {
        for (auto *hook : hooks())
            hook->finishReplacement(context, candidate);
    }

}
