#include "KeySignatureTrackAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor::Internal {
    KeySignatureTrackAddOn::KeySignatureTrackAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    KeySignatureTrackAddOn::~KeySignatureTrackAddOn() = default;

    void KeySignatureTrackAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "KeySignatureTrack", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)}
            });
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.arrangementPanel.additionalTracks.keySignature", o->property("keySignatureTrackComponent").value<QQmlComponent *>());
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.pianoRollPanel.additionalTracks.keySignature", o->property("keySignatureTrackComponent").value<QQmlComponent *>());
        }
    }

    void KeySignatureTrackAddOn::extensionsInitialized() {
    }

    bool KeySignatureTrackAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}

#include "moc_KeySignatureTrackAddOn.cpp"
