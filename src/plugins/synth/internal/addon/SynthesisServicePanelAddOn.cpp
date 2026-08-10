#include "SynthesisServicePanelAddOn.h"

#include <QAbstractItemModel>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <synth/internal/ServiceStatusModel.h>
#include <synth/internal/SynthService.h>

namespace Synth::Internal {

    SynthesisServicePanelAddOn::SynthesisServicePanelAddOn(QObject *parent)
        : Core::WindowInterfaceAddOn(parent),
          m_service(SynthService::instance()),
          m_model(new ServiceStatusModel(m_service, this)) {
        connect(m_service, &SynthService::refreshingChanged,
                this, &SynthesisServicePanelAddOn::refreshingChanged);
    }

    SynthesisServicePanelAddOn::~SynthesisServicePanelAddOn() = default;

    void SynthesisServicePanelAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(),
                                QStringLiteral("DiffScope.Synth"),
                                QStringLiteral("SynthesisServicePanel"), this);
        if (component.isError())
            qFatal() << component.errorString();
        auto object = component.createWithInitialProperties({
            {QStringLiteral("addOn"), QVariant::fromValue(this)},
        }, component.creationContext());
        if (!object)
            qFatal() << component.errorString();
        object->setParent(this);
        windowInterface->actionContext()->addAction(
            QStringLiteral("org.diffscope.synth.panel.services"),
            object->property("panelComponent").value<QQmlComponent *>());
    }

    void SynthesisServicePanelAddOn::extensionsInitialized() {
    }

    bool SynthesisServicePanelAddOn::delayedInitialize() {
        return Core::WindowInterfaceAddOn::delayedInitialize();
    }

    QAbstractItemModel *SynthesisServicePanelAddOn::serviceModel() const {
        return m_model;
    }

    bool SynthesisServicePanelAddOn::refreshing() const {
        return m_service->refreshing();
    }

    void SynthesisServicePanelAddOn::refreshAll() {
        m_service->refreshAll();
    }

}

#include "moc_SynthesisServicePanelAddOn.cpp"
