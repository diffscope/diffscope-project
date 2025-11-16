#include "ArrangementAddOn.h"

#include <QKeyEvent>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ArrangementPanelInterface.h>
#include <visualeditor/internal/EditorPreference.h>

namespace VisualEditor::Internal {
    ArrangementAddOn::ArrangementAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ArrangementAddOn::~ArrangementAddOn() = default;

    void ArrangementAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->window()->installEventFilter(this);
        new ArrangementPanelInterface(windowInterface);
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
            });
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.arrangement", o->property("arrangementPanelComponent").value<QQmlComponent *>());
        }

        // TODO
        windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.pianoRoll", new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "PianoRollPanel", this));
        windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.mixer", new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "MixerPanel", this));
    }

    void ArrangementAddOn::extensionsInitialized() {}

    bool ArrangementAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    ArrangementPanelInterface *ArrangementAddOn::arrangementPanelInterface() const {
        return ArrangementPanelInterface::of(windowHandle()->cast<Core::ProjectWindowInterface>());
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
                    if (keyEvent->key() == Qt::Key_Shift) {
                        arrangementPanelInterface()->setSnapTemporarilyDisabled(keyEvent->type() == QEvent::KeyPress);
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
