#include "TempoTrackAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor::Internal {
    TempoTrackAddOn::TempoTrackAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    TempoTrackAddOn::~TempoTrackAddOn() = default;

    void TempoTrackAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
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
        }
    }

    void TempoTrackAddOn::extensionsInitialized() {
    }

    bool TempoTrackAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}

#include "moc_TempoTrackAddOn.cpp"