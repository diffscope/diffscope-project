// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DocumentInterface.h"
#include "DocumentInterface_p.h"

#include <QThread>

#include <dspxmodelORM/EntityObject.h>
#include <dspxmodelORM/Model.h>

#include <coreplugin/DspxDocument.h>

#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/Storage.h>
#include <batchprocess/internal/CoreModuleExtension.h>

namespace BatchProcess {
    namespace {
        DocumentInterface *s_instance{};
    }

    DocumentInterface::DocumentInterface(QObject *parent) : QObject(parent), d_ptr(new DocumentInterfacePrivate(this)) {
        Q_ASSERT(!s_instance);
        s_instance = this;
    }

    DocumentInterface::~DocumentInterface() {
        Q_D(DocumentInterface);
        const auto documents = d->documents.values();
        for (auto document : documents) if (document) unregisterDocument(document);
        s_instance = nullptr;
    }

    DocumentInterface *DocumentInterface::instance() {
        return s_instance;
    }

    bool DocumentInterface::registerDocument(Core::DspxDocument *document) {
        Q_D(DocumentInterface);
        if (!document || QThread::currentThread() != thread() || document->thread() != thread()) {
            return false;
        }
        auto *model = document->model();
        if (d->documents.value(model) == document) {
            return true;
        }
        d->documents.insert(model, document);
        d->generations.insert(model, ++d->nextGeneration);
        connect(document, &QObject::destroyed, this, [this, model, document] {
            d_func()->documents.remove(model);
            d_func()->generations.remove(model);
            d_func()->storages.remove(document);
        });
        return true;
    }

    void DocumentInterface::unregisterDocument(Core::DspxDocument *document) {
        Q_D(DocumentInterface);
        if (!document || QThread::currentThread() != thread()) {
            return;
        }
        auto *model = document->model();
        if (d->extension) {
            d->extension->releaseDocument(document);
        }
        const auto storages = d->storages.take(document);
        for (auto storage : storages) delete storage.data();
        d->documents.remove(model);
        d->generations.remove(model);
    }

    Core::DspxDocument *DocumentInterface::documentForObject(QObject *object) const {
        Q_D(const DocumentInterface);
        if (!object || QThread::currentThread() != thread()) {
            return nullptr;
        }
        if (auto *document = qobject_cast<Core::DspxDocument *>(object)) {
            return d->documents.value(document->model()) == document ? document : nullptr;
        }
        for (auto *parent = object; parent; parent = parent->parent()) {
            if (auto *document = qobject_cast<Core::DspxDocument *>(parent)) {
                return d->documents.value(document->model()) == document ? document : nullptr;
            }
            if (auto *entity = qobject_cast<dspx::EntityObject *>(parent)) {
                return d->documents.value(entity->model());
            }
        }
        return nullptr;
    }

    QJSValue DocumentInterface::toJavaScriptValue(ScriptExecutionContext *context, Core::DspxDocument *document) {
        return registerDocument(document) ? wrapObject(context, document) : QJSValue();
    }

    QJSValue DocumentInterface::toJavaScriptValue(ScriptExecutionContext *context, dspx::Model *model) {
        return wrapObject(context, model);
    }

    QJSValue DocumentInterface::toJavaScriptValue(ScriptExecutionContext *context, dspx::EntityObject *entity) {
        return wrapObject(context, entity);
    }

    QJSValue DocumentInterface::wrapObject(ScriptExecutionContext *context, QObject *object) {
        Q_D(DocumentInterface);
        if (!context || !context->engine() || !d->extension || !documentForObject(object)) {
            return {};
        }
        return d->extension->wrap(context, object);
    }

    QObject *DocumentInterface::unwrapObject(ScriptExecutionContext *context, const QJSValue &value) const {
        Q_D(const DocumentInterface);
        return context && d->extension && QThread::currentThread() == thread() ? d->extension->unwrap(context, value) : nullptr;
    }

    bool DocumentInterface::fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, Core::DspxDocument **document) const {
        if (!document) {
            return false;
        }
        *document = qobject_cast<Core::DspxDocument *>(unwrapObject(context, value));
        return *document;
    }

    bool DocumentInterface::fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, dspx::Model **model) const {
        if (!model) {
            return false;
        }
        *model = qobject_cast<dspx::Model *>(unwrapObject(context, value));
        return *model;
    }

    bool DocumentInterface::fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, dspx::EntityObject **entity) const {
        if (!entity) {
            return false;
        }
        *entity = qobject_cast<dspx::EntityObject *>(unwrapObject(context, value));
        return *entity;
    }
}

#include "moc_DocumentInterface.cpp"
