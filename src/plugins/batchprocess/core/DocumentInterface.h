// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_DOCUMENTINTERFACE_H
#define DIFFSCOPE_BATCHPROCESS_DOCUMENTINTERFACE_H

#include <QJSValue>
#include <QObject>
#include <QScopedPointer>

#include <batchprocess/batchprocessglobal.h>

namespace Core {
    class DspxDocument;
}

namespace dspx {
    class EntityObject;
    class Model;
}

namespace BatchProcess {
    namespace Internal {
        class BatchProcessPlugin;
        class CoreModuleExtension;
        class CoreModuleBridge;
    }

    class DocumentInterfacePrivate;
    class ScriptExecutionContext;

    /** Provides non-owning document interoperation on the application thread. */
    class BATCH_PROCESS_EXPORT DocumentInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DocumentInterface)
    public:
        ~DocumentInterface() override;
        static DocumentInterface *instance();

        /** Registers a document without transferring ownership. Repeated registration is idempotent. */
        bool registerDocument(Core::DspxDocument *document);
        /** Aborts owned script transactions and permanently invalidates existing wrappers for this registration. */
        void unregisterDocument(Core::DspxDocument *document);
        /** Returns the registered document associated with a live model object, or nullptr. */
        Core::DspxDocument *documentForObject(QObject *object) const;

        /** Registers and wraps a document in the specified engine. Returns undefined on failure. */
        QJSValue toJavaScriptValue(ScriptExecutionContext *context, Core::DspxDocument *document);
        /** Wraps a model from a registered document. Returns undefined on failure. */
        QJSValue toJavaScriptValue(ScriptExecutionContext *context, dspx::Model *model);
        /** Wraps an attached entity from a registered document. Returns undefined on failure. */
        QJSValue toJavaScriptValue(ScriptExecutionContext *context, dspx::EntityObject *entity);
        /** Wraps a supported model object or container from a registered document. Returns undefined on failure. */
        QJSValue wrapObject(ScriptExecutionContext *context, QObject *object);

        /** Resolves an authentic, live wrapper from the same engine. Returns nullptr on failure. */
        QObject *unwrapObject(ScriptExecutionContext *context, const QJSValue &value) const;
        bool fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, Core::DspxDocument **document) const;
        bool fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, dspx::Model **model) const;
        bool fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, dspx::EntityObject **entity) const;

    private:
        friend class Internal::BatchProcessPlugin;
        friend class Internal::CoreModuleExtension;
        friend class Internal::CoreModuleBridge;
        explicit DocumentInterface(QObject *parent = nullptr);
        QScopedPointer<DocumentInterfacePrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_BATCHPROCESS_DOCUMENTINTERFACE_H
