#include "PianoRollAddOn.h"

#include <QKeyEvent>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/internal/EditorPreference.h>
#include <visualeditor/internal/AdditionalTrackLoader.h>
#include <visualeditor/internal/ScrollAddOn.h>

namespace VisualEditor::Internal {

    PianoRollAddOn::PianoRollAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    PianoRollAddOn::~PianoRollAddOn() = default;

    void PianoRollAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->window()->installEventFilter(this);
        m_additionalTrackLoader = new AdditionalTrackLoader("org.diffscope.visualeditor.pianoRollPanel.additionalTrackWidgets", this);
        auto pianoRollPanelInterface = new PianoRollPanelInterface(this, windowInterface);
        m_additionalTrackLoader->setContextObject(pianoRollPanelInterface);

        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "PianoRollAddOnActions");
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
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "PianoRollPanel", this);
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
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.pianoRoll", o->property("pianoRollPanelComponent").value<QQmlComponent *>());
        }
    }

    void PianoRollAddOn::extensionsInitialized() {
    }

    bool PianoRollAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    PianoRollPanelInterface *PianoRollAddOn::pianoRollPanelInterface() const {
        return PianoRollPanelInterface::of(windowHandle()->cast<Core::ProjectWindowInterface>());
    }

    AdditionalTrackLoader *PianoRollAddOn::additionalTrackLoader() const {
        return m_additionalTrackLoader;
    }

    bool PianoRollAddOn::eventFilter(QObject *watched, QEvent *event) {
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
                        pianoRollPanelInterface()->setSnapTemporarilyDisabled(keyEvent->type() == QEvent::KeyPress);
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
                    pianoRollPanelInterface()->setSnapTemporarilyDisabled(false);
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

    bool PianoRollAddOn::altPressed() const {
        return m_altPressed;
    }

    bool PianoRollAddOn::isTrackSelectorVisible() const {
        return m_trackSelectorVisible;
    }

    void PianoRollAddOn::setTrackSelectorVisible(bool visible) {
        if (m_trackSelectorVisible != visible) {
            m_trackSelectorVisible = visible;
            Q_EMIT trackSelectorVisibleChanged();
        }
    }

}

#include "moc_PianoRollAddOn.cpp"
