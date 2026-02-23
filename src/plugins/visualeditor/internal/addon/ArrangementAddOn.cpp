#include "ArrangementAddOn.h"

#include <QKeyEvent>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ArrangementPanelInterface.h>
#include <visualeditor/internal/EditorPreference.h>
#include <visualeditor/internal/AdditionalTrackLoader.h>
#include <visualeditor/internal/ScrollAddOn.h>

namespace VisualEditor::Internal {
    ArrangementAddOn::ArrangementAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ArrangementAddOn::~ArrangementAddOn() = default;

    void ArrangementAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->window()->installEventFilter(this);
        m_additionalTrackLoader = new AdditionalTrackLoader("org.diffscope.visualeditor.arrangementPanel.additionalTrackWidgets", this);
        auto arrangementPanelInterface = new ArrangementPanelInterface(this, windowInterface);
        m_additionalTrackLoader->setContextObject(arrangementPanelInterface);
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "ArrangementAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        }
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "ArrangementPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
                {"scrollAddOn", QVariant::fromValue(windowInterface->getFirstObject<ScrollAddOn>())},
            });
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.arrangement", o->property("arrangementPanelComponent").value<QQmlComponent *>());
        }
    }

    void ArrangementAddOn::extensionsInitialized() {}

    bool ArrangementAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    ArrangementPanelInterface *ArrangementAddOn::arrangementPanelInterface() const {
        return ArrangementPanelInterface::of(windowHandle()->cast<Core::ProjectWindowInterface>());
    }
    AdditionalTrackLoader *ArrangementAddOn::additionalTrackLoader() const {
        return m_additionalTrackLoader;
    }
    bool ArrangementAddOn::altPressed() const {
        return m_altPressed;
    }
    bool ArrangementAddOn::eventFilter(QObject *watched, QEvent *event) {
        if (watched == windowHandle()->window()) {
            switch (event->type()) {
                case QEvent::KeyPress:
                case QEvent::KeyRelease: {
                    if (!EditorPreference::enableTemporarySnapOff()) {
                        break;
                    }
                    auto keyEvent = static_cast<QKeyEvent *>(event);
                    if (keyEvent->isAutoRepeat()) {
                        break;
                    }
                    if (keyEvent->key() == Qt::Key_Shift) {
                        arrangementPanelInterface()->setSnapTemporarilyDisabled(keyEvent->type() == QEvent::KeyPress);
                    } else if (keyEvent->key() == Qt::Key_Alt) {
                        auto p = keyEvent->type() == QEvent::KeyPress;
                        if (m_altPressed != p) {
                            m_altPressed = p;
                            Q_EMIT altPressedChanged();
                        }
                    }
                    break;
                }
                case QEvent::FocusOut: {
                    arrangementPanelInterface()->setSnapTemporarilyDisabled(false);
                    if (m_altPressed) {
                        m_altPressed = false;
                        Q_EMIT altPressedChanged();
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return WindowInterfaceAddOn::eventFilter(watched, event);
    }
}

#include "moc_ArrangementAddOn.cpp"
