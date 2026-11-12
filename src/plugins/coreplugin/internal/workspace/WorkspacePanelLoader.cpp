// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WorkspacePanelLoader.h"

#include <QDebug>
#include <QJSEngine>
#include <QJSValue>
#include <QMetaType>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QVariantList>
#include <QVariantMap>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/DockingPanel.h>

namespace Core::Internal {
    static QJSValue stateToScriptValue(QJSEngine *engine, const QVariant &state) {
        if (state.typeId() == QMetaType::QVariantMap || state.typeId() == QMetaType::QVariantHash) {
            const auto map = state.toMap();
            auto object = engine->newObject();
            for (auto it = map.cbegin(); it != map.cend(); ++it)
                object.setProperty(it.key(), stateToScriptValue(engine, it.value()));
            return object;
        }
        if (state.typeId() == QMetaType::QVariantList || state.typeId() == QMetaType::QStringList) {
            const auto list = state.toList();
            auto array = engine->newArray(quint32(list.size()));
            for (qsizetype i = 0; i < list.size(); ++i)
                array.setProperty(quint32(i), stateToScriptValue(engine, list[i]));
            return array;
        }
        return engine->toScriptValue(state);
    }

    QQuickItem *WorkspacePanelLoader::create(QQmlComponent *component, SVS::DockingPanel *panel, QString *error) {
        if (!component) {
            *error = tr("The panel component is not registered.");
            return nullptr;
        }
        if (!component->isReady()) {
            *error = component->errorString().isEmpty() ? tr("The panel component is not ready.") : component->errorString();
            return nullptr;
        }

        QPointer<QQmlComponent> guard(component);
        auto object = component->createWithInitialProperties({{"panelKey", panel->key()}, {"panel", QVariant::fromValue(panel)}}, component->creationContext());
        if (!object) {
            if (guard)
                *error = guard->errorString();
            return nullptr;
        }

        auto item = qobject_cast<QQuickItem *>(object);
        if (!item || object->property("panel").value<SVS::DockingPanel *>() != panel) {
            *error = tr("The panel component must provide a DockingPane.");
            delete object;
            return nullptr;
        }
        return item;
    }

    QQuickItem *WorkspacePanelLoader::createFallback(SVS::DockingPanel *panel, const QString &title, bool componentRegistered, const QString &error) {
        QQmlComponent fallback(RuntimeInterface::qmlEngine(), "DiffScope.Core", "LoadingFailedFallbackPanel");
        if (fallback.isError()) {
            qFatal() << fallback.errorString();
        }
        auto object = fallback.createWithInitialProperties({
            {"panelKey", panel->key()},
            {"panel", QVariant::fromValue(panel)},
            {"panelTitle", title},
            {"componentRegistered", componentRegistered},
            {"errorString", error},
        });
        if (!object) {
            qFatal() << fallback.errorString();
        }
        auto item = qobject_cast<QQuickItem *>(object);
        if (!item) {
            qFatal("LoadingFailedFallbackPanel in DiffScope.Core is not QQuickItem");
        }
        return item;
    }

    void WorkspacePanelLoader::loadState(QQuickItem *item, const QVariant &state) {
        if (!item)
            return;
        auto context = QQmlEngine::contextForObject(item);
        if (!context || !context->isValid())
            return;
        auto engine = RuntimeInterface::qmlEngine();
        auto object = engine->newQObject(item);
        auto function = object.property("loadState");
        if (!function.isCallable())
            return;

        function.callWithInstance(object, {state.isValid() ? stateToScriptValue(engine, state) : QJSValue(QJSValue::UndefinedValue)});
    }

    QVariant WorkspacePanelLoader::saveState(QQuickItem *item, QString *error) {
        if (!item)
            return {};
        auto context = QQmlEngine::contextForObject(item);
        if (!context || !context->isValid())
            return {};
        auto object = RuntimeInterface::qmlEngine()->newQObject(item);
        auto function = object.property("saveState");
        if (!function.isCallable())
            return {};

        auto result = function.callWithInstance(object);
        if (result.isError()) {
            if (error)
                *error = item->property("panelKey").toString() + " [saveState]: " + result.toString();
            return {};
        }
        return result.toVariant();
    }
}

#include "moc_WorkspacePanelLoader.cpp"

