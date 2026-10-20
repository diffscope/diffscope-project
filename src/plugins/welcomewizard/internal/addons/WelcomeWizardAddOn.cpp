// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WelcomeWizardAddOn.h"

#include <memory>

#include <QEventLoop>
#include <QLoggingCategory>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <coreplugin/CoreInterface.h>

#include <welcomewizard/internal/WelcomeWizardPlugin.h>

namespace WelcomeWizard {
    Q_STATIC_LOGGING_CATEGORY(lcWelcomeWizardAddOn, "diffscope.welcomewizard.welcomewizardaddon")

    WelcomeWizardAddOn::WelcomeWizardAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    WelcomeWizardAddOn::~WelcomeWizardAddOn() = default;

    static QPointer<QQuickWindow> m_window;
    static WelcomeWizardPlugin *m_plugin = nullptr;

    void WelcomeWizardAddOn::setPlugin(WelcomeWizardPlugin *plugin) {
        m_plugin = plugin;
    }

    QQuickWindow *WelcomeWizardAddOn::window() {
        if (!m_window) {
            auto actionContext = Core::CoreInterface::actionContext();
            const auto entries = actionContext->registry()->layouts().adjacencyMap().value("org.diffscope.welcomewizard.pages");
            QList<QQuickItem *> pages;
            for (const auto &entry : entries) {
                if (entry.type() != QAK::ActionLayoutEntry::Action) {
                    qCWarning(lcWelcomeWizardAddOn) << "Skipping welcome wizard entry" << entry.id() << "of type" << entry.type() << "because it is not an action.";
                    continue;
                }
                auto component = actionContext->action(entry.id());
                if (!component) {
                    qCWarning(lcWelcomeWizardAddOn) << "Skipping welcome wizard action" << entry.id() << "because no component is registered.";
                    continue;
                }
                if (component->isError()) {
                    qCWarning(lcWelcomeWizardAddOn) << "Skipping welcome wizard action" << entry.id() << component->errorString();
                    continue;
                }
                std::unique_ptr<QObject> object(component->create(component->creationContext()));
                if (!object) {
                    qCWarning(lcWelcomeWizardAddOn) << "Skipping welcome wizard action" << entry.id() << component->errorString();
                    continue;
                }
                auto item = qobject_cast<QQuickItem *>(object.get());
                if (!item) {
                    qCWarning(lcWelcomeWizardAddOn) << "Skipping welcome wizard action" << entry.id() << "because its component is not QQuickItem.";
                    continue;
                }
                actionContext->attachActionInfo(entry.id(), item);
                pages.append(item);
                Q_UNUSED(object.release());
            }
            {
                QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.WelcomeWizard", "WelcomeWizardDialog");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.createWithInitialProperties({{"pages", QVariant::fromValue(pages)}});
                if (!o) {
                    qFatal() << component.errorString();
                }
                m_window = qobject_cast<QQuickWindow *>(o);
                if (!m_window) {
                    qFatal("WelcomeWizardDialog in DiffScope.WelcomeWizard is not QQuickWindow");
                }
                o->setParent(m_plugin);
            }
            for (auto item : pages) {
                item->setParent(m_window);
            }
        }
        return m_window;
    }

    void WelcomeWizardAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.WelcomeWizard", "WelcomeWizardActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}
        });
        if (!o) {
            qFatal() << component.errorString();
        }
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void WelcomeWizardAddOn::extensionsInitialized() {
    }

    bool WelcomeWizardAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    void WelcomeWizardAddOn::execWelcomeWizard() {
        QEventLoop eventLoop;
        auto win = window();
        connect(win, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        win->show();
        eventLoop.exec();
        win->deleteLater();
    }
}
