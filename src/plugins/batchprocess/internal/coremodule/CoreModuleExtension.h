// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_COREMODULEEXTENSION_H
#define DIFFSCOPE_BATCHPROCESS_COREMODULEEXTENSION_H

#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QPointer>

class QJSEngine;

namespace Core {
    class DspxDocument;
}

namespace BatchProcess {
    class BatchProcessInterface;
    class DocumentInterface;
    class ScriptExecutionContext;
}

namespace BatchProcess::Internal {
    class CoreModuleBridge;
    class CoreModuleExtension : public QObject {
        Q_OBJECT
    public:
        explicit CoreModuleExtension(BatchProcessInterface *runtime, DocumentInterface *documents, QObject *parent = nullptr);
        ~CoreModuleExtension() override;
        QJSValue wrap(ScriptExecutionContext *context, QObject *object) const;
        QObject *unwrap(ScriptExecutionContext *context, const QJSValue &value) const;
        void releaseDocument(Core::DspxDocument *document);

    private:
        void install(QJSEngine *engine, ScriptExecutionContext *context);
        QHash<QJSEngine *, QJSValue> m_bundles;
        QHash<QJSEngine *, QPointer<CoreModuleBridge>> m_bridges;
        QPointer<DocumentInterface> m_documents;
    };
}

#endif // DIFFSCOPE_BATCHPROCESS_COREMODULEEXTENSION_H
