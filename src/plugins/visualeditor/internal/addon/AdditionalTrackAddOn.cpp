#include "AdditionalTrackAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace VisualEditor::Internal {
    AdditionalTrackAddOn::AdditionalTrackAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    AdditionalTrackAddOn::~AdditionalTrackAddOn() = default;

    void AdditionalTrackAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        
        // Label Track
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "LabelTrack", this);
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
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.arrangementPanel.additionalTracks.label", o->property("labelTrackComponent").value<QQmlComponent *>());
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.pianoRollPanel.additionalTracks.label", o->property("labelTrackComponent").value<QQmlComponent *>());
        }

        // Tempo Track
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "TempoTrack", this);
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
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.arrangementPanel.additionalTracks.tempo", o->property("tempoTrackComponent").value<QQmlComponent *>());
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.pianoRollPanel.additionalTracks.tempo", o->property("tempoTrackComponent").value<QQmlComponent *>());
        }

        // Key Signature Track
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

        // Clip Track
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "ClipTrack", this);
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
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.pianoRollPanel.additionalTracks.clip", o->property("clipTrackComponent").value<QQmlComponent *>());
        }
    }

    void AdditionalTrackAddOn::extensionsInitialized() {
    }

    bool AdditionalTrackAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}

#include "moc_AdditionalTrackAddOn.cpp"
