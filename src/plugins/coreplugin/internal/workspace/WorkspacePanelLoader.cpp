// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WorkspacePanelLoader.h"

#include <memory>
#include <utility>

#include <QDebug>
#include <QJSEngine>
#include <QJSValue>
#include <QMetaType>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QVariantList>
#include <QVariantMap>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/DockingPanel.h>

#include <coreplugin/ProjectWindowInterface.h>

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

    WorkspacePanelLoader::WorkspacePanelLoader(ProjectWindowInterface *window, QObject *parent) : QObject(parent), m_window(window) {}

    void WorkspacePanelLoader::cancel() {
        m_cancelled = true;
    }

    void WorkspacePanelLoader::load(SVS::DockingPanel *panel, bool placeholder, Completion completion) {
        if (m_cancelled) {
            completion(nullptr, {});
            return;
        }
        auto completed = std::make_shared<bool>(false);
        completion = [completed, callback = std::move(completion)](QQuickItem *item, const QString &error) {
            if (std::exchange(*completed, true))
                return;
            callback(item, error);
        };

        auto component = m_window->actionContext()->action(panel->key());
        if (component && component->isLoading()) {
            auto connection = std::make_shared<QMetaObject::Connection>();
            QPointer<QQmlComponent> guard(component);
            QPointer<SVS::DockingPanel> panelGuard(panel);
            *connection = connect(component, &QQmlComponent::statusChanged, this, [this, guard, panelGuard, placeholder, completion, connection](QQmlComponent::Status status) {
                if (status == QQmlComponent::Loading)
                    return;
                disconnect(*connection);
                if (panelGuard)
                    create(guard, panelGuard, placeholder, completion);
                else
                    completion(nullptr, tr("The panel request expired."));
            });
            connect(component, &QObject::destroyed, this, [panelGuard, completion] {
                if (panelGuard)
                    completion(nullptr, tr("The panel component was destroyed."));
            });
            return;
        }

        create(component, panel, placeholder, std::move(completion));
    }

    void WorkspacePanelLoader::create(QQmlComponent *component, SVS::DockingPanel *panel, bool placeholder, Completion completion) {
        if (m_cancelled) {
            completion(nullptr, {});
            return;
        }
        QString error;
        QObject *object = nullptr;
        if (!component)
            error = tr("The panel component is not registered.");
        else if (!component->isReady())
            error = component->errorString().isEmpty() ? tr("The panel component is not ready.") : component->errorString();
        else {
            object = component->createWithInitialProperties({{"panelKey", panel->key()}, {"panel", QVariant::fromValue(panel)}}, component->creationContext());
            if (!object)
                error = component->errorString();
        }

        auto item = qobject_cast<QQuickItem *>(object);
        if (object && (!item || object->property("panel").value<SVS::DockingPanel *>() != panel)) {
            error = tr("The panel component must provide a DockingPane.");
            delete object;
            object = nullptr;
            item = nullptr;
        }

        if (!item && placeholder) {
            QQmlComponent fallback(RuntimeInterface::qmlEngine(), "DiffScope.Core", "LoadingFailedFallbackPanel");
            if (fallback.isError()) {
                qFatal() << fallback.errorString();
            }
            object = fallback.createWithInitialProperties({{"panelKey", panel->key()}, {"panel", QVariant::fromValue(panel)}, {"actionId", panel->key()}, {"actionText", panel->title()}, {"componentRegistered", bool(component)}, {"errorString", error}});
            item = qobject_cast<QQuickItem *>(object);
            if (!item) {
                qFatal() << "LoadingFailedFallbackPanel could not be created as a QQuickItem." << fallback.errorString();
            }
        }

        if (item) {
            item->setParent(parent());
            QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
            item->setVisible(false);
            if (component)
                m_window->actionContext()->attachActionInfo(panel->key(), item);
            panel->setContentItem(item);
            panel->setHeaderItem(item->property("header").value<QQuickItem *>());
        }

        completion(item, error);
    }

    bool WorkspacePanelLoader::invokeStateMethod(QQuickItem *item, const QString &method, const QVariant &state, bool withState, QString *error) {
        if (!item || (withState && !state.isValid()))
            return true;
        auto engine = RuntimeInterface::qmlEngine();
        auto object = engine->newQObject(item);
        auto function = object.property(method);
        if (!function.isCallable())
            return true;

        auto result = function.callWithInstance(object, withState ? QJSValueList{stateToScriptValue(engine, state)} : QJSValueList{});
        if (!result.isError() && !(result.isBool() && !result.toBool()))
            return true;
        if (error)
            *error = item->property("panelKey").toString() + " [" + method + "]: " + (result.isError() ? result.toString() : tr("The panel rejected the workspace state in %1.").arg(method));
        return false;
    }

    bool WorkspacePanelLoader::validateState(QQuickItem *item, const QVariant &state, QString *error) {
        return invokeStateMethod(item, "validateState", state, true, error);
    }

    bool WorkspacePanelLoader::prepareState(QQuickItem *item, const QVariant &state, QString *error) {
        return invokeStateMethod(item, "prepareState", state, true, error);
    }

    bool WorkspacePanelLoader::applyState(QQuickItem *item, const QVariant &state, QString *error) {
        return invokeStateMethod(item, "loadState", state, true, error);
    }

    bool WorkspacePanelLoader::finishState(QQuickItem *item, QString *error) {
        return invokeStateMethod(item, "finishState", {}, false, error);
    }

    bool WorkspacePanelLoader::abortState(QQuickItem *item, QString *error) {
        return invokeStateMethod(item, "abortState", {}, false, error);
    }

    bool WorkspacePanelLoader::loadState(QQuickItem *item, const QVariant &state, QString *error) {
        if (!validateState(item, state, error))
            return false;
        if (!prepareState(item, state, error) || !applyState(item, state, error)) {
            QString abortError;
            if (!abortState(item, &abortError) && error)
                *error += '\n' + abortError;
            return false;
        }
        return finishState(item, error);
    }

    QVariant WorkspacePanelLoader::saveState(QQuickItem *item, QString *error) {
        if (!item)
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

