#include "ArrangementAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace VisualEditor::Internal {
    ArrangementAddOn::ArrangementAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {}

    ArrangementAddOn::~ArrangementAddOn() {}

    void ArrangementAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.arrangement", new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "ArrangementPanel", this));

        // TODO
        windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.pianoRoll", new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "PianoRollPanel", this));
        windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.mixer", new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "MixerPanel", this));
    }

    void ArrangementAddOn::extensionsInitialized() {}

    bool ArrangementAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}
