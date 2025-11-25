#include "LabelTrackAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace VisualEditor::Internal {
    LabelTrackAddOn::LabelTrackAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    LabelTrackAddOn::~LabelTrackAddOn() = default;

    void LabelTrackAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
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
        }
    }

    void LabelTrackAddOn::extensionsInitialized() {
    }

    bool LabelTrackAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}

#include "moc_LabelTrackAddOn.cpp"