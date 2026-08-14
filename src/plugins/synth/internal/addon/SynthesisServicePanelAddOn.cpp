#include "SynthesisServicePanelAddOn.h"

#include <QAbstractItemModel>
#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QSaveFile>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <synth/SynthInterface.h>
#include <synth/SynthesisTask.h>
#include <synth/SynthesisTaskManager.h>
#include <synth/internal/ServiceStatusModel.h>
#include <synth/internal/SynthService.h>

namespace Synth::Internal {

    SynthesisServicePanelAddOn::SynthesisServicePanelAddOn(QObject *parent)
        : Core::WindowInterfaceAddOn(parent),
          m_service(SynthService::instance()),
          m_taskManager(SynthInterface::instance()->taskManager()),
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

    QUrl SynthesisServicePanelAddOn::diagnosticsDirectoryUrl() const {
        return QUrl::fromLocalFile(m_taskManager->diagnosticsDirectory());
    }

    void SynthesisServicePanelAddOn::refreshAll() {
        m_service->refreshAll();
    }

    bool SynthesisServicePanelAddOn::copyDiagnosticRequest(QObject *taskObject, int exchangeIndex) {
        auto task = qobject_cast<SynthesisTask *>(taskObject);
        if (!task || exchangeIndex < 0 || exchangeIndex >= task->diagnostics().size()) {
            return false;
        }
        const auto diagnostics = task->diagnostics();
        QGuiApplication::clipboard()->setText(diagnostics.at(exchangeIndex).toMap().value(QStringLiteral("requestBody")).toString());
        return true;
    }

    bool SynthesisServicePanelAddOn::copyDiagnosticResponse(QObject *taskObject, int exchangeIndex) {
        auto task = qobject_cast<SynthesisTask *>(taskObject);
        if (!task || exchangeIndex < 0 || exchangeIndex >= task->diagnostics().size()) {
            return false;
        }
        const auto diagnostics = task->diagnostics();
        QGuiApplication::clipboard()->setText(diagnostics.at(exchangeIndex).toMap().value(QStringLiteral("responseBody")).toString());
        return true;
    }

    bool SynthesisServicePanelAddOn::exportDiagnostics(QObject *taskObject, const QUrl &fileUrl) {
        auto task = qobject_cast<SynthesisTask *>(taskObject);
        if (!task || task->diagnosticFilePath().isEmpty() || !fileUrl.isLocalFile()) {
            return false;
        }
        QFile source(task->diagnosticFilePath());
        if (!source.open(QIODevice::ReadOnly)) {
            return false;
        }
        QSaveFile destination(fileUrl.toLocalFile());
        if (!destination.open(QIODevice::WriteOnly)) {
            return false;
        }
        const auto bytes = source.readAll();
        return destination.write(bytes) == bytes.size() && destination.commit();
    }

    void SynthesisServicePanelAddOn::clearDiagnostics() {
        m_taskManager->clearDiagnostics();
    }

}

#include "moc_SynthesisServicePanelAddOn.cpp"
