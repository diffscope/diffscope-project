// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CoreModuleExtension.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

#include <QColor>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaProperty>
#include <QPointF>
#include <QPointer>
#include <QSequentialIterable>
#include <QSet>
#include <QVariant>
#include <QWindow>

#include <CoreApi/filelocker.h>
#include <CoreApi/windowsystem.h>

#include <opendspx/model.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeSignature.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/ArchitectureInfo.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/DefaultLyricManager.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/FreeParameterSelectionModel.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/OpenSaveProjectFileScenario.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SingerRegistry.h>
#include <coreplugin/TrackColorSchema.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/DocumentInterface.h>
#include <batchprocess/FileSystemAccessInterface.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/ShellInterface.h>
#include <batchprocess/Storage.h>
#include <batchprocess/StorageInterface.h>
#include <batchprocess/internal/CoreModelOperations.h>
#include <batchprocess/private/DocumentInterface_p.h>

#include <transactional/TransactionController.h>

namespace BatchProcess::Internal {
    Q_STATIC_LOGGING_CATEGORY(lcCoreModule, "diffscope.batchprocess.coremodule")

    class CoreModuleBridge : public QObject {
        Q_OBJECT
    public:
        CoreModuleBridge(DocumentInterface *documents, ScriptExecutionContext *context, QJSEngine *engine)
            : QObject(engine), m_documents(documents), m_context(context), m_engine(engine) {}
        ~CoreModuleBridge() override { finish(); }

        Q_INVOKABLE QString kind(QObject *object) const;
        Q_INVOKABLE QString generation(QObject *object) const;
        Q_INVOKABLE bool valid(QObject *object, const QString &generation) const;
        Q_INVOKABLE bool sameDocument(QObject *left, QObject *right) const { return document(left) && document(left) == document(right); }
        Q_INVOKABLE QString identity(QObject *object) const { auto *entity = qobject_cast<dspx::EntityObject *>(object); return entity ? QString::number(entity->handle().d) : QString(); }
        Q_INVOKABLE QJSValue model(QObject *object) { return guarded(object, QStringLiteral("model"), false, [&] { return success(raw(document(object)->model())); }); }
        Q_INVOKABLE QJSValue read(QObject *object, const QString &property);
        Q_INVOKABLE QJSValue write(QObject *object, const QString &property, const QJSValue &value);
        Q_INVOKABLE QJSValue items(QObject *container);
        Q_INVOKABLE QJSValue allocate(QObject *container, const QString &kind, bool original);
        Q_INVOKABLE QJSValue attach(QObject *container, QObject *entity, int index, const QString &key);
        Q_INVOKABLE QJSValue transfer(QObject *container, QObject *entity, int index);
        Q_INVOKABLE QJSValue rotate(QObject *container, int leftIndex, int middleIndex, int rightIndex);
        Q_INVOKABLE QJSValue destroy(QObject *entity);
        Q_INVOKABLE QJSValue replace(QObject *object, const QJSValue &data, bool initializing);
        Q_INVOKABLE QJSValue snapshot(QObject *object);
        Q_INVOKABLE QJSValue splice(QObject *array, int index, int count, const QJSValue &values);
        Q_INVOKABLE QJSValue begin(QObject *object);
        Q_INVOKABLE QJSValue transaction(const QString &token, const QString &operation, const QString &name);
        Q_INVOKABLE QJSValue complete(QObject *object);
        Q_INVOKABLE QJSValue command(QObject *object, const QString &operation, const QJSValue &arguments);
        Q_INVOKABLE QJSValue service(const QString &operation, const QJSValue &arguments);
        void start(const QJSValue &wrap);
        void finish(Core::DspxDocument *only = nullptr);

    private:
        struct Edit {
            QPointer<Core::DspxDocument> document;
            Core::TransactionController::TransactionId id{Core::TransactionController::TransactionId::Invalid};
            QString token;
            QHash<quint64, QPointer<dspx::EntityObject>> existingDetached;
        };
        QJSValue success(const QJSValue &value = QJSValue()) const;
        QJSValue record() const;
        QJSValue array(uint size) const;
        QJSValue failure(const QString &code, const QString &operation, const QString &message = {}) const;
        QJSValue raw(QObject *object);
        QJSValue values(const QList<QObject *> &objects);
        Core::DspxDocument *document(QObject *object) const;
        Edit *edit(QObject *object);
        bool cleanup(Edit &edit);
        void watch(Core::DspxDocument *document);
        bool orphaned(const Edit &edit) const;
        void cleanScope(Core::DspxDocument *document);
        void registerProject(Core::ProjectWindowInterface *project);
        QJSValue guarded(QObject *object, const QString &operation, bool mutation, const std::function<QJSValue()> &callback);
        QJSValue propertyValue(const QVariant &value);
        QJSValue registry(const QString &operation, const QJSValue &arguments);
        QJSValue parameter(const Core::ParameterInfo &info, const QString &id, bool transform);
        QJSValue projectCommand(Core::ProjectWindowInterface *project, const QString &operation, const QJSValue &arguments);
        QPointer<DocumentInterface> m_documents;
        QPointer<ScriptExecutionContext> m_context;
        QJSEngine *m_engine;
        QHash<dspx::Model *, Edit> m_edits;
        QHash<dspx::Model *, Edit> m_scopes;
        QSet<Core::TransactionController *> m_controllers;
        QSet<Core::ProjectWindowInterface *> m_projects;
        QJSValue m_associatedProject{QJSValue::NullValue};
        quint64 m_nextToken{};
    };

    QJSValue CoreModuleBridge::record() const {
        auto result = m_engine->newObject();
        result.setPrototype(QJSValue(QJSValue::NullValue));
        return result;
    }

    QJSValue CoreModuleBridge::array(uint size) const {
        auto result = m_engine->newArray(size);
        result.setPrototype(QJSValue(QJSValue::NullValue));
        return result;
    }

    QJSValue CoreModuleBridge::success(const QJSValue &value) const {
        auto result = record();
        result.setProperty(QStringLiteral("ok"), true);
        result.setProperty(QStringLiteral("value"), value);
        return result;
    }

    QJSValue CoreModuleBridge::failure(const QString &code, const QString &operation, const QString &message) const {
        auto result = record();
        result.setProperty(QStringLiteral("ok"), false);
        result.setProperty(QStringLiteral("code"), code);
        result.setProperty(QStringLiteral("operation"), operation);
        result.setProperty(QStringLiteral("message"), message.isEmpty() ? DocumentInterface::tr("The script document operation is not available in the current state.") : message);
        return result;
    }

    QJSValue CoreModuleBridge::raw(QObject *object) {
        if (!object) return QJSValue(QJSValue::NullValue);
        QJSEngine::setObjectOwnership(object, QJSEngine::CppOwnership);
        return m_engine->newQObject(object);
    }

    QJSValue CoreModuleBridge::values(const QList<QObject *> &objects) {
        auto result = array(static_cast<uint>(objects.size()));
        for (qsizetype i = 0; i < objects.size(); ++i) result.setProperty(static_cast<uint>(i), raw(objects.at(i)));
        return result;
    }

    QJSValue CoreModuleBridge::propertyValue(const QVariant &value) {
        if (!value.isValid()) return QJSValue(QJSValue::NullValue);
        if (value.metaType() == QMetaType::fromType<std::nullptr_t>()) return QJSValue(QJSValue::NullValue);
        if (value.metaType().flags().testFlag(QMetaType::PointerToQObject)) return raw(value.value<QObject *>());
        if (value.metaType() == QMetaType::fromType<QVariantMap>()) {
            auto result = record();
            const auto map = value.toMap();
            for (auto it = map.begin(); it != map.end(); ++it) result.setProperty(it.key(), propertyValue(it.value()));
            return result;
        }
        if (value.metaType() == QMetaType::fromType<QPointF>()) {
            const auto point = value.toPointF();
            auto result = record();
            result.setProperty(QStringLiteral("x"), point.x());
            result.setProperty(QStringLiteral("y"), point.y());
            return result;
        }
        if (value.canConvert<QSequentialIterable>()) {
            const auto sequence = value.value<QSequentialIterable>();
            auto result = array(static_cast<uint>(sequence.size()));
            uint index = 0;
            for (const auto &item : sequence) result.setProperty(index++, propertyValue(item));
            return result;
        }
        return m_engine->toScriptValue(value);
    }

    void CoreModuleBridge::registerProject(Core::ProjectWindowInterface *project) {
        if (!project || project->isEffectivelyClosed()) return;
        m_documents->registerDocument(project->projectDocumentContext()->document());
        if (m_projects.contains(project)) return;
        m_projects.insert(project);
        connect(project, &Core::ExecutiveInterface::loadingStateChanged, this, [this, project] {
            if (m_documents && project->isEffectivelyClosed()) m_documents->unregisterDocument(project->projectDocumentContext()->document());
        });
        connect(project, &QObject::destroyed, this, [this, project] { m_projects.remove(project); });
    }

    void CoreModuleBridge::start(const QJSValue &wrap) {
        m_associatedProject = QJSValue(QJSValue::NullValue);
        auto *project = m_context ? m_context->projectWindowInterface() : nullptr;
        if (!project || project->isEffectivelyClosed()) return;
        registerProject(project);
        m_associatedProject = wrap.call({raw(project)});
    }

    Core::DspxDocument *CoreModuleBridge::document(QObject *object) const {
        if (auto *project = qobject_cast<Core::ProjectWindowInterface *>(object)) return project->projectDocumentContext()->document();
        if (auto *timeline = qobject_cast<Core::ProjectTimeline *>(object)) {
            for (auto *window : Core::CoreInterface::windowSystem()->windows()) if (auto *project = qobject_cast<Core::ProjectWindowInterface *>(window); project && project->projectTimeline() == timeline) return project->projectDocumentContext()->document();
        }
        return m_documents ? m_documents->documentForObject(object) : nullptr;
    }

    QString CoreModuleBridge::kind(QObject *object) const {
        if (qobject_cast<Core::DspxDocument *>(object)) return QStringLiteral("Document");
        if (qobject_cast<Core::ProjectWindowInterface *>(object)) return QStringLiteral("Project");
        if (qobject_cast<Core::ProjectTimeline *>(object)) return QStringLiteral("Timeline");
        if (qobject_cast<Core::FreeParameterSelectionModel *>(object)) return QStringLiteral("FreeParameterSelection");
        if (qobject_cast<dspx::SelectionModel *>(object)) return QStringLiteral("Selection");
        const auto name = object ? QString::fromLatin1(object->metaObject()->className()).section(QStringLiteral("::"), -1) : QString();
        if (name.endsWith(QStringLiteral("SelectionModel"))) return name;
        return modelObjectKind(object);
    }

    QString CoreModuleBridge::generation(QObject *object) const {
        auto *doc = document(object);
        return doc && m_documents && m_documents->d_func()->documents.contains(doc->model()) ? QString::number(m_documents->d_func()->generations.value(doc->model())) : QString();
    }

    bool CoreModuleBridge::valid(QObject *object, const QString &stamp) const {
        if (!object || stamp.isEmpty() || stamp != generation(object)) return false;
        if (auto *entity = qobject_cast<dspx::EntityObject *>(object)) return liveEntity(entity) && attachedObject(entity);
        if (owningModel(object)) return attachedObject(object);
        return document(object);
    }

    CoreModuleBridge::Edit *CoreModuleBridge::edit(QObject *object) {
        const auto *doc = document(object);
        if (!doc) return nullptr;
        auto it = m_edits.find(doc->model());
        return it != m_edits.end() && it->document && it->document->transactionController()->isExclusiveToTransaction() ? &it.value() : nullptr;
    }

    QJSValue CoreModuleBridge::guarded(QObject *object, const QString &operation, bool mutation, const std::function<QJSValue()> &callback) {
        if (!m_context || !m_context->action()) return failure(QStringLiteral("invalidState"), operation);
        if (object && !document(object)) return failure(QStringLiteral("invalidObject"), operation);
        if (auto *entity = qobject_cast<dspx::EntityObject *>(object); entity && !liveEntity(entity)) return failure(QStringLiteral("invalidObject"), operation);
        if (mutation && !edit(object)) return failure(QStringLiteral("transactionConflict"), operation);
        QPointer<Core::DspxDocument> target(document(object));
        if (target) watch(target);
        try {
            const auto value = callback();
            if (target && !m_edits.contains(target->model())) cleanScope(target);
            return value;
        } catch (const std::exception &error) {
            qCWarning(lcCoreModule) << "Native document operation failed" << operation << error.what();
            if (target) finish(target);
            return failure(QStringLiteral("invalidData"), operation, QString::fromUtf8(error.what()));
        } catch (...) {
            qCWarning(lcCoreModule) << "Native document operation failed" << operation;
            if (target) finish(target);
            return failure(QStringLiteral("invalidData"), operation);
        }
    }

    bool CoreModuleBridge::cleanup(Edit &state) {
        if (!state.document) return true;
        auto *model = state.document->model();
        for (const auto &pointer : modelEntities(model)) {
            auto *entity = pointer.data();
            if (!entity || state.existingDetached.value(entity->handle().d) == entity || !liveEntity(entity) || attachedObject(entity)) continue;
            if (!model->destroyItem(entity)) return false;
        }
        return true;
    }

    void CoreModuleBridge::watch(Core::DspxDocument *doc) {
        if (!doc || m_scopes.contains(doc->model())) return;
        auto *controller = doc->transactionController();
        if (!m_controllers.contains(controller)) {
            m_controllers.insert(controller);
            auto *model = doc->model();
            connect(controller, &Core::TransactionController::transactionAborted, this, [this, model] { m_edits.remove(model); });
            connect(controller, &Core::TransactionController::transactionCommitted, this, [this, model] { m_edits.remove(model); });
            connect(controller, &QObject::destroyed, this, [this, controller, model] { m_controllers.remove(controller); m_edits.remove(model); m_scopes.remove(model); });
        }
        Edit state;
        state.document = doc;
        for (const auto &pointer : modelEntities(doc->model())) if (pointer && !attachedObject(pointer)) state.existingDetached.insert(pointer->handle().d, pointer);
        m_scopes.insert(doc->model(), state);
    }

    bool CoreModuleBridge::orphaned(const Edit &state) const {
        if (!state.document) return false;
        for (const auto &pointer : modelEntities(state.document->model())) if (pointer && state.existingDetached.value(pointer->handle().d) != pointer && liveEntity(pointer) && !attachedObject(pointer)) return true;
        return false;
    }

    void CoreModuleBridge::cleanScope(Core::DspxDocument *doc) {
        if (!doc || m_edits.contains(doc->model())) return;
        auto state = m_scopes.value(doc->model());
        if (!orphaned(state) || doc->transactionController()->isExclusiveToTransaction()) return;
        const auto id = doc->transactionController()->beginTransaction();
        if (id == Core::TransactionController::TransactionId::Invalid) return;
        try {
            if (cleanup(state)) doc->transactionController()->commitTransaction(id, DocumentInterface::tr("Cleaning up script objects"));
            else doc->transactionController()->abortTransaction(id);
        } catch (...) {
            doc->transactionController()->abortTransaction(id);
            throw;
        }
    }

    void CoreModuleBridge::finish(Core::DspxDocument *only) {
        if (!only) m_associatedProject = QJSValue(QJSValue::NullValue);
        const auto models = m_edits.keys();
        for (auto *model : models) {
            auto state = m_edits.value(model);
            if (only && state.document != only) continue;
            m_edits.remove(model);
            if (!state.document) continue;
            try {
                cleanup(state);
                state.document->transactionController()->abortTransaction(state.id);
                // Original phonemes are outside the persistent transaction.
                for (const auto &pointer : modelEntities(model)) {
                    auto *phoneme = qobject_cast<dspx::Phoneme *>(pointer.data());
                    if (phoneme && phoneme->role() == dspx::Phoneme::Original && !attachedObject(phoneme) && state.existingDetached.value(phoneme->handle().d) != phoneme) model->destroyItem(phoneme);
                }
            } catch (const std::exception &error) {
                qCCritical(lcCoreModule) << "Script transaction cleanup failed" << error.what();
                try { state.document->transactionController()->abortTransaction(state.id); }
                catch (...) { qCCritical(lcCoreModule) << "Script transaction abort failed"; }
            } catch (...) {
                qCCritical(lcCoreModule) << "Script transaction cleanup failed";
                try { state.document->transactionController()->abortTransaction(state.id); }
                catch (...) { qCCritical(lcCoreModule) << "Script transaction abort failed"; }
            }
        }
        const auto scopedModels = m_scopes.keys();
        for (auto *model : scopedModels) {
            const auto state = m_scopes.value(model);
            if (only && state.document != only) continue;
            try { if (state.document) cleanScope(state.document); }
            catch (const std::exception &error) { qCCritical(lcCoreModule) << "Script scope cleanup failed" << error.what(); }
            catch (...) { qCCritical(lcCoreModule) << "Script scope cleanup failed"; }
            m_scopes.remove(model);
        }
    }

    QJSValue CoreModuleBridge::begin(QObject *object) {
        return guarded(object, QStringLiteral("beginTransaction"), false, [&] {
            auto *doc = document(object);
            if (!doc || doc->transactionController()->isExclusiveToTransaction()) return failure(QStringLiteral("transactionConflict"), QStringLiteral("beginTransaction"));
            Edit state;
            state.document = doc;
            for (const auto &pointer : modelEntities(doc->model())) if (pointer && !attachedObject(pointer)) state.existingDetached.insert(pointer->handle().d, pointer);
            state.id = doc->transactionController()->beginTransaction();
            if (state.id == Core::TransactionController::TransactionId::Invalid) return failure(QStringLiteral("transactionConflict"), QStringLiteral("beginTransaction"));
            state.token = QString::number(++m_nextToken);
            m_edits.insert(doc->model(), state);
            return success(state.token);
        });
    }

    QJSValue CoreModuleBridge::transaction(const QString &token, const QString &operation, const QString &name) {
        for (auto it = m_edits.begin(); it != m_edits.end(); ++it) {
            if (it->token != token || !it->document) continue;
            if (operation == QStringLiteral("active")) return success(true);
            auto *doc = it->document.data();
            return guarded(doc, operation, true, [&] {
                if (operation == QStringLiteral("abort")) { finish(doc); return success(); }
                if (operation != QStringLiteral("commit") || name.trimmed().isEmpty()) return failure(QStringLiteral("invalidData"), operation);
                auto state = m_edits.value(doc->model());
                if (!cleanup(state)) return failure(QStringLiteral("invalidData"), operation);
                const auto committed = doc->transactionController()->commitTransaction(state.id, name);
                m_edits.remove(doc->model());
                return committed ? success() : failure(QStringLiteral("transactionConflict"), operation);
            });
        }
        return operation == QStringLiteral("active") ? success(false) : failure(QStringLiteral("invalidState"), operation);
    }

    QJSValue CoreModuleBridge::complete(QObject *object) {
        return guarded(object, QStringLiteral("complete"), true, [&] { return cleanup(*edit(object)) ? success() : failure(QStringLiteral("invalidData"), QStringLiteral("complete")); });
    }

    QJSValue CoreModuleBridge::items(QObject *container) {
        return guarded(container, QStringLiteral("items"), false, [&] { return success(values(modelItems(container))); });
    }

    QJSValue CoreModuleBridge::allocate(QObject *container, const QString &type, bool original) {
        return guarded(container, QStringLiteral("create"), true, [&] {
            return success(raw(allocateEntity(document(container)->model(), type, original)));
        });
    }

    QJSValue CoreModuleBridge::attach(QObject *container, QObject *object, int index, const QString &key) {
        return guarded(container, QStringLiteral("create"), true, [&] {
            const auto ok = attachEntity(container, qobject_cast<dspx::EntityObject *>(object), index, key);
            return ok ? success() : failure(QStringLiteral("invalidData"), QStringLiteral("create"));
        });
    }

    QJSValue CoreModuleBridge::transfer(QObject *container, QObject *object, int index) {
        return guarded(container, QStringLiteral("moveHere"), true, [&] {
            return transferEntity(container, qobject_cast<dspx::EntityObject *>(object), index) ? success() : failure(QStringLiteral("invalidData"), QStringLiteral("moveHere"));
        });
    }

    QJSValue CoreModuleBridge::destroy(QObject *object) {
        return guarded(object, QStringLiteral("delete"), true, [&] {
            auto *entity = qobject_cast<dspx::EntityObject *>(object);
            return entity && entity->model()->destroyItem(entity) ? success() : failure(QStringLiteral("invalidObject"), QStringLiteral("delete"));
        });
    }

    QJSValue CoreModuleBridge::rotate(QObject *container, int leftIndex, int middleIndex, int rightIndex) {
        return guarded(container, QStringLiteral("rotate"), true, [&] {
            return rotateContainer(container, leftIndex, middleIndex, rightIndex) ? success() : failure(QStringLiteral("invalidData"), QStringLiteral("rotate"));
        });
    }

    QJSValue CoreModuleBridge::replace(QObject *object, const QJSValue &data, bool initializing) {
        return guarded(object, QStringLiteral("fromOpenDSPX"), true, [&] {
            if (!importDSPX(object, QJsonValue::fromVariant(data.toVariant()), !initializing)) return failure(QStringLiteral("invalidData"), QStringLiteral("fromOpenDSPX"));
            return success();
        });
    }

    QJSValue CoreModuleBridge::snapshot(QObject *object) {
        return guarded(object, QStringLiteral("toOpenDSPX"), false, [&] {
            if (auto *doc = qobject_cast<Core::DspxDocument *>(object)) object = doc->model();
            QJsonValue data;
            return exportDSPX(object, &data) ? success(propertyValue(data.toVariant())) : failure(QStringLiteral("invalidData"), QStringLiteral("toOpenDSPX"));
        });
    }

    QJSValue CoreModuleBridge::read(QObject *object, const QString &property) {
        return guarded(object, property, false, [&] {
            if (property == QStringLiteral("document") && qobject_cast<dspx::Model *>(object)) return success(raw(document(object)));
            if (property == QStringLiteral("handle")) {
                auto *entity = qobject_cast<dspx::EntityObject *>(object);
                return success(entity ? QJSValue(QString::number(entity->handle().d)) : QJSValue());
            }
            if (auto *project = qobject_cast<Core::ProjectWindowInterface *>(object)) {
                auto *context = project->projectDocumentContext();
                auto *doc = context->document();
                if (property == QStringLiteral("document")) return success(raw(doc));
                if (property == QStringLiteral("timeline")) return success(raw(project->projectTimeline()));
                if (property == QStringLiteral("window")) return success(ShellInterface::instance()->toJavaScriptValue(m_context, project));
                if (property == QStringLiteral("id")) return success(QString::number(reinterpret_cast<quintptr>(project), 16));
                if (property == QStringLiteral("filePath")) return success(context->fileLocker() && !context->fileLocker()->path().isEmpty() ? QJSValue(context->fileLocker()->path()) : QJSValue(QJSValue::NullValue));
                if (property == QStringLiteral("displayName")) return success(context->fileLocker() && !context->fileLocker()->path().isEmpty() ? QFileInfo(context->fileLocker()->path()).fileName() : context->defaultDocumentName());
                if (property == QStringLiteral("modified")) return success(doc->transactionController()->currentStep() != doc->transactionController()->cleanStep());
            }
            if (auto *clip = qobject_cast<dspx::AudioClip *>(object); clip && property == QStringLiteral("path")) {
                const auto path = clip->path();
                QByteArray bytes;
                QDataStream stream(&bytes, QIODevice::WriteOnly);
                stream.setVersion(QDataStream::Qt_5_15);
                stream << path.userData;
                return success(propertyValue(QVariantMap{{"absoluteDir", path.absoluteDir}, {"relativeDir", path.relativeDir}, {"fileName", path.fileName}, {"formatEntryClassName", path.formatEntryClassName}, {"serializedUserData", QString::fromLatin1(bytes.toBase64())}, {"digest", path.digest}}));
            }
            if (auto *parameter = qobject_cast<dspx::Parameter *>(object); parameter && property == QStringLiteral("parameterId")) {
                if (auto *map = parameter->parameterMap()) for (const auto &key : map->keys()) if (map->item(key) == parameter) return success(key);
                return success(QJSValue(QJSValue::NullValue));
            }
            const auto variant = object->property(property.toLatin1().constData());
            if (variant.metaType() == QMetaType::fromType<QJsonValue>()) return success(propertyValue(variant.value<QJsonValue>().toVariant()));
            return success(propertyValue(variant));
        });
    }

    QJSValue CoreModuleBridge::write(QObject *object, const QString &property, const QJSValue &value) {
        return guarded(object, property, true, [&] {
            if (auto *clip = qobject_cast<dspx::SingingClip *>(object); clip && property == QStringLiteral("sources") && value.isNull()) { clip->setSources(nullptr); return success(); }
            if (auto *clip = qobject_cast<dspx::AudioClip *>(object); clip && property == QStringLiteral("path")) {
                dspx::AudioPathInfo path;
                path.absoluteDir = value.property(QStringLiteral("absoluteDir")).toString();
                path.relativeDir = value.property(QStringLiteral("relativeDir")).toString();
                path.fileName = value.property(QStringLiteral("fileName")).toString();
                path.formatEntryClassName = value.property(QStringLiteral("formatEntryClassName")).toString();
                path.digest = value.property(QStringLiteral("digest")).toString();
                auto bytes = QByteArray::fromBase64(value.property(QStringLiteral("serializedUserData")).toString().toLatin1());
                QDataStream stream(&bytes, QIODevice::ReadOnly);
                stream.setVersion(QDataStream::Qt_5_15);
                stream >> path.userData;
                clip->setPath(path);
                return success();
            }
            const auto index = object->metaObject()->indexOfProperty(property.toLatin1().constData());
            if (index < 0) return failure(QStringLiteral("invalidData"), property);
            const auto meta = object->metaObject()->property(index);
            QVariant converted;
            if (meta.metaType() == QMetaType::fromType<QJsonValue>()) converted = QVariant::fromValue(QJsonValue::fromVariant(value.toVariant()));
            else if (meta.metaType() == QMetaType::fromType<QList<double>>()) {
                QList<double> list;
                for (uint i = 0; i < value.property(QStringLiteral("length")).toUInt(); ++i) list.append(value.property(i).toNumber());
                converted = QVariant::fromValue(list);
            } else converted = value.isQObject() ? QVariant::fromValue(value.toQObject()) : value.toVariant();
            if (converted.metaType() != meta.metaType() && !converted.convert(meta.metaType())) return failure(QStringLiteral("invalidData"), property);
            return meta.isWritable() && meta.write(object, converted) ? success() : failure(QStringLiteral("invalidData"), property);
        });
    }

    QJSValue CoreModuleBridge::splice(QObject *array, int index, int count, const QJSValue &values) {
        return guarded(array, QStringLiteral("splice"), true, [&] {
            bool ok = false;
            if (auto *samples = qobject_cast<dspx::FreeValueDataArray *>(array)) {
                QList<QVariant> items;
                for (uint i = 0; i < values.property(QStringLiteral("length")).toUInt(); ++i) items.append(values.property(i).isNull() ? QVariant() : QVariant(values.property(i).toInt()));
                ok = samples->splice(index, count, items);
            } else if (auto *points = qobject_cast<dspx::VibratoPointDataArray *>(array)) {
                QList<QPointF> items;
                for (uint i = 0; i < values.property(QStringLiteral("length")).toUInt(); ++i) {
                    const auto point = values.property(i);
                    items.append(QPointF(point.property(QStringLiteral("x")).toNumber(), point.property(QStringLiteral("y")).toNumber()));
                }
                ok = points->splice(index, count, items);
            }
            return ok ? success() : failure(QStringLiteral("invalidData"), QStringLiteral("splice"));
        });
    }

    class ParameterConversionBridge : public QObject {
        Q_OBJECT
    public:
        ParameterConversionBridge(Core::ParameterInfo info, QJSEngine *engine) : m_info(std::move(info)), m_engine(engine) {}
        Q_INVOKABLE QJSValue convert(const QString &operation, const QJSValue &input) const {
            auto result = m_engine->newObject();
            result.setPrototype(QJSValue(QJSValue::NullValue));
            try {
                result.setProperty(QStringLiteral("value"), convertValue(operation, input));
                result.setProperty(QStringLiteral("ok"), true);
            } catch (const std::exception &error) {
                result.setProperty(QStringLiteral("ok"), false);
                result.setProperty(QStringLiteral("code"), QStringLiteral("invalidData"));
                result.setProperty(QStringLiteral("operation"), operation);
                result.setProperty(QStringLiteral("message"), QString::fromUtf8(error.what()));
            } catch (...) {
                result.setProperty(QStringLiteral("ok"), false);
                result.setProperty(QStringLiteral("code"), QStringLiteral("invalidData"));
                result.setProperty(QStringLiteral("operation"), operation);
                result.setProperty(QStringLiteral("message"), DocumentInterface::tr("The script document operation is not available in the current state."));
            }
            return result;
        }
    private:
        QJSValue convertValue(const QString &operation, const QJSValue &input) const {
            if (operation == QStringLiteral("fromDSPXValue")) return input.isNull() ? QJSValue(QJSValue::NullValue) : QJSValue(Core::ParameterInfo::fromDspxModelValue(input.toInt()));
            auto number = input.toNumber();
            if (operation == QStringLiteral("toDSPXValue")) return Core::ParameterInfo::toDspxModelValue(std::isfinite(number) ? number : m_info.defaultValue);
            if (operation == QStringLiteral("fromDisplayValue")) {
                const auto result = m_info.invokeFromDisplayValue(number);
                return std::isfinite(result) ? QJSValue(std::clamp(result, 0.0, 1.0)) : QJSValue(QJSValue::NullValue);
            }
            if (input.isNull() || !std::isfinite(number)) return operation == QStringLiteral("formatValue") ? QJSValue(QString()) : QJSValue(QJSValue::NullValue);
            number = std::clamp(number, 0.0, 1.0);
            if (operation == QStringLiteral("formatValue")) return m_info.invokeToDisplayString(number);
            const auto result = m_info.invokeToDisplayValue(number);
            return std::isfinite(result) ? QJSValue(result) : QJSValue(QJSValue::NullValue);
        }
        Core::ParameterInfo m_info;
        QJSEngine *m_engine;
    };

    QJSValue CoreModuleBridge::parameter(const Core::ParameterInfo &info, const QString &id, bool transform) {
        auto result = record();
        result.setProperty(QStringLiteral("id"), id);
        result.setProperty(QStringLiteral("transform"), transform);
        result.setProperty(QStringLiteral("displayName"), info.displayName);
        result.setProperty(QStringLiteral("defaultValue"), info.defaultValue);
        result.setProperty(QStringLiteral("baselineValue"), info.baselineValue);
        result.setProperty(QStringLiteral("showDefaultValue"), info.showDefaultValue);
        result.setProperty(QStringLiteral("valueType"), info.valueType == Core::ParameterInfo::Absolute ? QStringLiteral("absolute") : QStringLiteral("relative"));
        const QStringList fills{QStringLiteral("none"), QStringLiteral("top"), QStringLiteral("bottom"), QStringLiteral("baseline")};
        result.setProperty(QStringLiteral("fillMode"), fills.value(info.fillMode));
        auto *conversion = new ParameterConversionBridge(info, m_engine);
        QJSEngine::setObjectOwnership(conversion, QJSEngine::JavaScriptOwnership);
        result.setProperty(QStringLiteral("conversion"), m_engine->newQObject(conversion));
        return result;
    }

    QJSValue CoreModuleBridge::registry(const QString &operation, const QJSValue &args) {
        auto *registry = Core::CoreInterface::singerRegistry();
        const auto architectureId = args.property(0).toString();
        if (operation == QStringLiteral("architectures")) return success(propertyValue(registry->architectureIds()));
        if (operation == QStringLiteral("singers")) return success(propertyValue(registry->singerIds(architectureId)));
        if (operation == QStringLiteral("architecture")) {
            if (!registry->containsArchitecture(architectureId)) return success(QJSValue(QJSValue::NullValue));
            const auto info = registry->architectureInfo(architectureId);
            auto result = record();
            result.setProperty(QStringLiteral("id"), architectureId);
            result.setProperty(QStringLiteral("name"), info.name());
            result.setProperty(QStringLiteral("defaultExtra"), propertyValue(info.defaultExtra().isUndefined() ? QVariant::fromValue(nullptr) : info.defaultExtra().toVariant()));
            const auto parameters = info.parameters();
            auto items = array(static_cast<uint>(parameters.size()));
            uint index = 0;
            for (auto it = parameters.begin(); it != parameters.end(); ++it) items.setProperty(index++, parameter(it.value(), it.key(), false));
            result.setProperty(QStringLiteral("parameters"), items);
            return success(result);
        }
        if (operation == QStringLiteral("singer")) {
            const auto id = args.property(1).toString();
            if (!registry->containsSinger(architectureId, id)) return success(QJSValue(QJSValue::NullValue));
            const auto info = registry->singerInfo(architectureId, id);
            auto result = propertyValue(QVariantMap{{"architectureId", architectureId}, {"singerId", id}, {"name", info.name()}, {"defaultLanguage", info.defaultLanguage()}, {"supportedParameters", info.supportedParameters()}, {"mixGroup", info.mixGroup()}, {"defaultExtra", info.defaultExtra().isUndefined() ? QVariant::fromValue(nullptr) : info.defaultExtra().toVariant()}});
            result.setProperty(QStringLiteral("avatarUrl"), info.avatarUrl().isEmpty() ? QJSValue(QJSValue::NullValue) : QJSValue(info.avatarUrl().toString()));
            result.setProperty(QStringLiteral("backgroundUrl"), info.backgroundUrl().isEmpty() ? QJSValue(QJSValue::NullValue) : QJSValue(info.backgroundUrl().toString()));
            auto languages = array(static_cast<uint>(info.languages().size()));
            const auto map = info.languages();
            uint index = 0;
            for (auto it = map.begin(); it != map.end(); ++it) languages.setProperty(index++, propertyValue(QVariantMap{{"id", it.key()}, {"name", it->name}, {"defaultLyric", it->defaultLyric}}));
            result.setProperty(QStringLiteral("languages"), languages);
            return success(result);
        }
        if (operation == QStringLiteral("parameter")) {
            const auto id = args.property(1).toString();
            const auto transform = args.property(2).toBool();
            const auto parameters = registry->architectureInfo(architectureId).parameters();
            if (id != QStringLiteral("pitch") && (!registry->containsArchitecture(architectureId) || !parameters.contains(id))) return success(QJSValue(QJSValue::NullValue));
            return success(parameter(transform ? Core::transformParameterInfo() : id == QStringLiteral("pitch") ? Core::pitchParameterInfo() : parameters.value(id), id, transform));
        }
        return failure(QStringLiteral("unsupportedOperation"), operation);
    }

    QJSValue CoreModuleBridge::projectCommand(Core::ProjectWindowInterface *project, const QString &operation, const QJSValue &args) {
        auto *doc = project->projectDocumentContext()->document();
        if (doc->transactionController()->isTransactionActive()) return failure(QStringLiteral("transactionConflict"), operation);
        if (operation == QStringLiteral("close")) {
            QPointer<Core::DspxDocument> guard(doc);
            const auto closed = project->window()->close();
            if (closed && guard) m_documents->unregisterDocument(guard);
            return success(closed);
        }
        if (operation == QStringLiteral("triggerAction")) {
            if (!m_edits.isEmpty()) return failure(QStringLiteral("transactionConflict"), operation);
            return success(project->triggerAction(args.property(0).toString()));
        }
        if (operation == QStringLiteral("save")) return success(project->save());
        const auto target = args.property(0);
        if (target.isUndefined()) return success(operation == QStringLiteral("saveAs") ? project->saveAs() : project->saveCopy());
        FileSystemHandle handle;
        FileSystemError error;
        auto *files = FileSystemAccessInterface::instance();
        if (!files->fromJavaScriptValue(m_context, target, &handle, FileSystemHandle::File, &error) || !files->validateHandle(m_context, handle, FileAccessMode::Write, FileSystemHandle::File, &error)) return failure(QStringLiteral("invalidData"), operation, error.message);
        return success(operation == QStringLiteral("saveAs") ? project->projectDocumentContext()->saveAs(handle.path()) : project->projectDocumentContext()->saveCopy(handle.path()));
    }

    QJSValue CoreModuleBridge::service(const QString &operation, const QJSValue &args) {
        return guarded(nullptr, operation, false, [&] {
            if (operation == QStringLiteral("active")) return success();
            if (operation == QStringLiteral("architectures") || operation == QStringLiteral("architecture") || operation == QStringLiteral("singers") || operation == QStringLiteral("singer") || operation == QStringLiteral("parameter")) return registry(operation, args);
            if (operation == QStringLiteral("trackColors")) {
                QVariantList colors;
                for (const auto &color : Core::CoreInterface::trackColorSchema()->colors()) {
                    const auto css = color.alpha() == 255 ? color.name(QColor::HexRgb) : QStringLiteral("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(QString::number(color.alphaF(), 'g', 8));
                    colors.append(QVariantMap{{"red", color.redF()}, {"green", color.greenF()}, {"blue", color.blueF()}, {"alpha", color.alphaF()}, {"css", css}});
                }
                return success(propertyValue(colors));
            }
            if (operation == QStringLiteral("lyric")) return success(Core::CoreInterface::defaultLyricManager()->getDefaultLyricForSingingClip(qobject_cast<dspx::SingingClip *>(args.property(0).toQObject())));
            if (operation == QStringLiteral("showHome")) { Core::CoreInterface::showHome(); return success(); }
            if (operation == QStringLiteral("homeWindow")) {
                for (auto *window : Core::CoreInterface::windowSystem()->windows()) if (auto *home = qobject_cast<Core::HomeWindowInterface *>(window)) return success(ShellInterface::instance()->toJavaScriptValue(m_context, home));
                return success(QJSValue(QJSValue::NullValue));
            }
            if (operation == QStringLiteral("project")) return success(m_associatedProject);
            if (operation == QStringLiteral("projects")) {
                QList<QObject *> projects;
                for (auto *window : Core::CoreInterface::windowSystem()->windows()) if (auto *project = qobject_cast<Core::ProjectWindowInterface *>(window); project && !project->isEffectivelyClosed()) {
                    registerProject(project);
                    projects.append(project);
                }
                return success(values(projects));
            }
            if (operation == QStringLiteral("newProjectFromOpenDSPX")) {
                auto context = std::make_unique<Core::ProjectDocumentContext>();
                // Parse into a temporary native model before initializing a project window.
                auto temporary = std::make_unique<Core::DspxDocument>();
                const auto transaction = temporary->transactionController()->beginTransaction();
                if (!importDSPX(temporary->model(), QJsonValue::fromVariant(args.property(0).toVariant()), !args.property(2).toBool())) {
                    temporary->transactionController()->abortTransaction(transaction);
                    return failure(QStringLiteral("invalidData"), operation);
                }
                const auto native = temporary->model()->toOpenDSPX();
                temporary->transactionController()->abortTransaction(transaction);
                temporary.reset();
                const auto requestedName = args.property(1).toString();
                const auto name = requestedName.isEmpty() ? Core::CoreInterface::tr("Untitled") + QStringLiteral(".dspx") : requestedName;
                if (!context->newFile(native, name, false)) return success(QJSValue(QJSValue::NullValue));
                auto *project = Core::CoreInterface::createProjectWindow(context.release());
                registerProject(project);
                return success(raw(project));
            }
            if (operation == QStringLiteral("newProject")) {
                auto *project = Core::CoreInterface::newFile(m_context->windowInterface() ? m_context->windowInterface()->window() : nullptr);
                if (project && !args.property(0).toString().isEmpty()) project->projectDocumentContext()->setDefaultDocumentName(args.property(0).toString());
                registerProject(project);
                return success(raw(project));
            }
            if (operation == QStringLiteral("openProject") || operation == QStringLiteral("newProjectFromTemplate")) {
                QString path;
                if (!args.property(0).isUndefined()) {
                    FileSystemHandle handle;
                    FileSystemError error;
                    auto *files = FileSystemAccessInterface::instance();
                    if (!files->fromJavaScriptValue(m_context, args.property(0), &handle, FileSystemHandle::File, &error) || !files->validateHandle(m_context, handle, FileAccessMode::Read, FileSystemHandle::File, &error)) return failure(QStringLiteral("invalidData"), operation, error.message);
                    path = handle.path();
                }
                auto *parent = m_context->windowInterface() ? m_context->windowInterface()->window() : nullptr;
                if (path.isEmpty()) { Core::OpenSaveProjectFileScenario picker; picker.setWindow(parent); path = picker.openProjectFile(); }
                if (path.isEmpty()) return success(QJSValue(QJSValue::NullValue));
                auto *project = operation == QStringLiteral("openProject") ? Core::CoreInterface::openProjectFile(path, parent) : Core::CoreInterface::newFileFromTemplate(path, parent);
                if (project && operation == QStringLiteral("newProjectFromTemplate") && !args.property(1).toString().isEmpty()) project->projectDocumentContext()->setDefaultDocumentName(args.property(1).toString());
                registerProject(project);
                return success(raw(project));
            }
            return failure(QStringLiteral("unsupportedOperation"), operation);
        });
    }

    QJSValue CoreModuleBridge::command(QObject *object, const QString &operation, const QJSValue &args) {
        return guarded(object, operation, false, [&] {
            auto *doc = document(object);
            if (!doc) return failure(QStringLiteral("invalidObject"), operation);
            if (auto *project = qobject_cast<Core::ProjectWindowInterface *>(object)) {
                if (operation == QStringLiteral("sessionStorage")) {
                    const auto id = args.property(0).toString();
                    auto &bucket = m_documents->d_func()->storages[doc][id];
                    if (!bucket) bucket = StorageInterface::instance()->createStorage(id, {}, doc);
                    return success(StorageInterface::instance()->toJavaScriptValue(m_context, bucket));
                }
                return projectCommand(project, operation, args);
            }
            if (operation == QStringLiteral("history")) {
                auto *history = doc->transactionController();
                auto value = record();
                const auto position = history->currentStep();
                const auto length = history->totalSteps();
                value.setProperty(QStringLiteral("position"), position);
                value.setProperty(QStringLiteral("length"), length);
                value.setProperty(QStringLiteral("cleanPosition"), history->cleanStep());
                value.setProperty(QStringLiteral("canUndo"), position > 0);
                value.setProperty(QStringLiteral("canRedo"), position < length);
                value.setProperty(QStringLiteral("undoName"), position > 0 ? QJSValue(history->stepName(position - 1)) : QJSValue(QJSValue::NullValue));
                value.setProperty(QStringLiteral("redoName"), position < length ? QJSValue(history->stepName(position)) : QJSValue(QJSValue::NullValue));
                return success(value);
            }
            if (operation == QStringLiteral("goToHistory")) {
                if (doc->transactionController()->isTransactionActive()) return failure(QStringLiteral("transactionConflict"), operation);
                doc->transactionController()->setCurrentStep(args.property(0).toInt());
                return success();
            }
            if (operation == QStringLiteral("documentState")) return success(propertyValue(QVariantMap{{"transactionActive", doc->transactionController()->isTransactionActive()}, {"canCopy", doc->anyItemsSelected()}, {"canPaste", doc->pasteAvailable()}, {"canBounceSelection", doc->canSafelyBounce()}, {"selectedClipSingerLayoutIdentical", doc->isClipSingerLayoutIdentical()}}));
            if (operation == QStringLiteral("selection")) {
                auto *selection = doc->selectionModel();
                auto *item = args.property(0).toQObject();
                const auto flags = dspx::SelectionModel::SelectionCommand(args.property(1).toInt());
                const auto type = static_cast<dspx::SelectionModel::SelectionType>(args.property(2).toInt());
                auto *hint = args.property(3).toQObject();
                if (item && document(item) != doc) return failure(QStringLiteral("invalidData"), operation);
                if (hint && document(hint) != doc) return failure(QStringLiteral("invalidData"), operation);
                selection->select(item, flags, type, hint);
                return success();
            }
            if (operation == QStringLiteral("freeContext")) {
                auto *clip = qobject_cast<dspx::SingingClip *>(args.property(0).toQObject());
                if (clip && document(clip) != doc) return failure(QStringLiteral("invalidData"), operation);
                doc->freeParameterSelectionModel()->setContext(clip, args.property(1).toString(), args.property(2).toString(), args.property(3).toInt() == 0 ? Core::FreeParameterSelectionModel::EditedLayer : Core::FreeParameterSelectionModel::TransformLayer);
                return success();
            }
            if (operation == QStringLiteral("freeRange")) { doc->freeParameterSelectionModel()->setRange(args.property(0).toInt(), args.property(1).toInt()); return success(); }
            if (operation == QStringLiteral("clearFreeRange")) { doc->freeParameterSelectionModel()->clear(); return success(); }
            if (operation == QStringLiteral("clearFreeContext")) { doc->freeParameterSelectionModel()->clearContext(); return success(); }
            if (operation == QStringLiteral("renameParameter")) {
                if (!edit(object)) return failure(QStringLiteral("transactionConflict"), operation);
                auto *map = qobject_cast<dspx::ParameterMap *>(object);
                return map && map->moveItem(args.property(0).toString(), map, args.property(1).toString()) ? success() : failure(QStringLiteral("invalidData"), operation);
            }
            if (auto *timeline = qobject_cast<Core::ProjectTimeline *>(object)) {
                if (operation == QStringLiteral("timelineSet")) {
                    const auto name = args.property(0).toString();
                    const auto value = args.property(1).toInt();
                    if (name == QStringLiteral("playhead")) timeline->setPosition(value);
                    else if (name == QStringLiteral("previousPlayhead")) timeline->setLastPosition(value);
                    else if (name == QStringLiteral("contentEndHint")) timeline->setRangeHint(value);
                    else return failure(QStringLiteral("invalidData"), operation);
                    return success();
                }
                if (operation == QStringLiteral("goTo")) { timeline->goTo(args.property(0).toInt()); return success(); }
                if (operation == QStringLiteral("timelineMaps")) {
                    QVariantList tempos;
                    const auto map = timeline->musicTimeline()->tempi();
                    for (auto it = map.begin(); it != map.end(); ++it) tempos.append(QVariantMap{{"position", it.key()}, {"value", it.value()}});
                    QVariantList signatures;
                    const auto signatureMap = timeline->musicTimeline()->timeSignatures();
                    for (auto it = signatureMap.begin(); it != signatureMap.end(); ++it) signatures.append(QVariantMap{{"index", it.key()}, {"numerator", it.value().numerator()}, {"denominator", it.value().denominator()}});
                    return success(propertyValue(QVariantMap{{"tempos", tempos}, {"timeSignatures", signatures}}));
                }
            }
            if (doc->transactionController()->isTransactionActive()) return failure(QStringLiteral("transactionConflict"), operation);
            if (operation == QStringLiteral("copy")) doc->copySelection(args.property(0).toInt());
            else if (operation == QStringLiteral("cut")) doc->cutSelection(args.property(0).toInt());
            else if (operation == QStringLiteral("paste")) doc->paste(args.property(0).toInt(), static_cast<Core::DspxDocument::PasteMode>(args.property(1).toInt()));
            else if (operation == QStringLiteral("deleteSelection")) doc->deleteSelection();
            else if (operation == QStringLiteral("splitSelectionAt")) doc->splitItems(args.property(0).toInt());
            else if (operation == QStringLiteral("bounceSelectionToClip")) {
                if (!doc->canSafelyBounce()) return failure(QStringLiteral("invalidData"), operation);
                doc->bounceToClip();
            } else return failure(QStringLiteral("unsupportedOperation"), operation);
            return success();
        });
    }

    CoreModuleExtension::CoreModuleExtension(BatchProcessInterface *runtime, DocumentInterface *documents, QObject *parent)
        : QObject(parent), m_documents(documents) {
        documents->d_func()->extension = this;
        const auto installed = runtime->registerEngineExtension(this, [this](QJSEngine *engine, ScriptExecutionContext *context) { install(engine, context); });
        const auto registered = runtime->registerModule(QStringLiteral("diffscope:core"), this, [this](ScriptExecutionContext *context) { return m_bundles.value(context->engine()).property(QStringLiteral("exports")); });
        const auto started = runtime->registerActionStartedCallback(this, [this](ScriptExecutionContext *context) {
            if (auto bridge = m_bridges.value(context->engine())) bridge->start(m_bundles.value(context->engine()).property(QStringLiteral("wrap")));
        });
        const auto finished = runtime->registerActionFinishedCallback(this, [this](ScriptExecutionContext *context) {
            if (auto bridge = m_bridges.value(context->engine())) bridge->finish();
        });
        if (!installed || !registered || !started || !finished) qFatal("Failed to register the Batch Process core module");
    }

    CoreModuleExtension::~CoreModuleExtension() {
        if (m_documents) m_documents->d_func()->extension = nullptr;
    }

    void CoreModuleExtension::install(QJSEngine *engine, ScriptExecutionContext *context) {
        QFile file(QStringLiteral(":/diffscope/batchprocess/internalscripts/core/core-module.js"));
        if (!file.open(QIODevice::ReadOnly)) qFatal() << file.errorString();
        const auto factory = engine->evaluate(QString::fromUtf8(file.readAll()), QStringLiteral("qrc:/diffscope/batchprocess/internalscripts/core/core-module.js"));
        if (!factory.isCallable() || factory.isError()) qFatal() << factory.toString();
        auto *bridge = new CoreModuleBridge(m_documents, context, engine);
        m_bridges.insert(engine, bridge);
        connect(engine, &QObject::destroyed, this, [this, engine] { m_bundles.remove(engine); m_bridges.remove(engine); });
        QFile validationFile(QStringLiteral(":/diffscope/batchprocess/internalscripts/core/core-validation.js"));
        if (!validationFile.open(QIODevice::ReadOnly)) qFatal() << validationFile.errorString();
        const auto validation = engine->evaluate(QString::fromUtf8(validationFile.readAll()), QStringLiteral("qrc:/diffscope/batchprocess/internalscripts/core/core-validation.js"));
        if (validation.isError() || !validation.isObject()) qFatal() << validation.toString();
        const auto bundle = factory.call({engine->newQObject(bridge), validation});
        if (bundle.isError() || !bundle.property(QStringLiteral("wrap")).isCallable() || !bundle.property(QStringLiteral("unwrap")).isCallable()) qFatal() << bundle.toString();
        m_bundles.insert(engine, bundle);
    }

    QJSValue CoreModuleExtension::wrap(ScriptExecutionContext *context, QObject *object) const {
        if (!context || !context->engine()) return {};
        const auto bundle = m_bundles.value(context->engine());
        const auto helper = bundle.property(QStringLiteral("wrap"));
        if (!helper.isCallable() || !object) return {};
        QJSEngine::setObjectOwnership(object, QJSEngine::CppOwnership);
        const auto result = helper.call({context->engine()->newQObject(object)});
        if (context->engine()->hasError()) { context->engine()->catchError(); return {}; }
        return result.isError() ? QJSValue() : result;
    }

    QObject *CoreModuleExtension::unwrap(ScriptExecutionContext *context, const QJSValue &value) const {
        if (!context || !context->engine()) return nullptr;
        const auto helper = m_bundles.value(context->engine()).property(QStringLiteral("unwrap"));
        if (!helper.isCallable()) return nullptr;
        const auto result = helper.call({value});
        if (context->engine()->hasError()) { context->engine()->catchError(); return nullptr; }
        return result.isQObject() ? result.toQObject() : nullptr;
    }

    void CoreModuleExtension::releaseDocument(Core::DspxDocument *document) {
        for (auto bridge : m_bridges) if (bridge) bridge->finish(document);
    }
}

#include "CoreModuleExtension.moc"
#include "moc_CoreModuleExtension.cpp"
