// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ControlPanelProvider.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QQmlEngine>
#include <QSettings>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <synth/SynthInterface.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcControlPanelProvider, "diffscope.synth.controlpanelprovider")

    ControlPanelProvider::ControlPanelProvider(QObject *parent) : QObject(parent) {
        connect(SynthInterface::instance(), &SynthInterface::controlPanelsChanged, this, &ControlPanelProvider::scheduleReload);
    }

    ControlPanelProvider::~ControlPanelProvider() = default;

    void ControlPanelProvider::classBegin() {
    }

    void ControlPanelProvider::componentComplete() {
        m_complete = true;
        reload();
    }

    QString ControlPanelProvider::architectureId() const {
        return m_architectureId;
    }

    void ControlPanelProvider::setArchitectureId(const QString &architectureId) {
        if (m_architectureId == architectureId)
            return;
        m_architectureId = architectureId;
        Q_EMIT architectureIdChanged();
        scheduleReload();
    }

    QUrl ControlPanelProvider::source() const {
        return m_source;
    }

    void ControlPanelProvider::setSource(const QUrl &source) {
        if (m_source == source)
            return;
        m_source = source;
        Q_EMIT sourceChanged();
        scheduleReload();
    }

    bool ControlPanelProvider::preview() const {
        return m_preview;
    }

    void ControlPanelProvider::setPreview(bool preview) {
        if (m_preview == preview)
            return;
        m_preview = preview;
        Q_EMIT previewChanged();
        scheduleReload();
    }

    QQmlComponent *ControlPanelProvider::architectureControlPanelComponent() const {
        return m_architecturePanel;
    }

    QQmlComponent *ControlPanelProvider::singerControlPanelComponent() const {
        return m_singerPanel;
    }

    bool ControlPanelProvider::loading() const {
        return m_loading;
    }

    bool ControlPanelProvider::failed() const {
        return m_failed;
    }

    void ControlPanelProvider::scheduleReload() {
        if (!m_complete || m_reloadPending)
            return;
        m_reloadPending = true;
        QMetaObject::invokeMethod(this, [this] {
            m_reloadPending = false;
            reload();
        }, Qt::QueuedConnection);
    }

    void ControlPanelProvider::reload() {
        m_architecturePanel = nullptr;
        m_singerPanel = nullptr;
        m_loading = false;
        m_failed = false;
        Q_EMIT panelsChanged();
        delete m_provider;
        m_provider = nullptr;
        delete m_providerComponent;
        m_providerComponent = nullptr;

        auto url = m_source;
        if (!m_preview) {
            auto settings = Core::RuntimeInterface::settings();
            settings->beginGroup(QStringLiteral("org.diffscope.synth"));
            const auto providers = QJsonDocument::fromJson(settings->value(QStringLiteral("architectureControlPanelProviders")).toByteArray()).object();
            settings->endGroup();
            url = QUrl(providers.value(m_architectureId).toString());
        }
        if (url.isEmpty()) {
            const auto interface = SynthInterface::instance();
            if (interface && interface->isBuiltInArchitecture(m_architectureId)) {
                m_architecturePanel = interface->builtInArchitecturePanel(m_architectureId);
                m_singerPanel = interface->builtInSingerPanel(m_architectureId);
                Q_EMIT panelsChanged();
            }
            return;
        }
        if (!url.isLocalFile() || !url.isValid()) {
            m_failed = true;
            Q_EMIT panelsChanged();
            return;
        }
        m_loading = true;
        Q_EMIT panelsChanged();
        m_providerComponent = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), url, this);
        connect(m_providerComponent, &QQmlComponent::statusChanged, this, &ControlPanelProvider::finishLoading);
        finishLoading();
    }

    void ControlPanelProvider::finishLoading() {
        if (!m_providerComponent || m_providerComponent->isLoading() || !m_loading)
            return;
        m_loading = false;
        if (m_providerComponent->isReady()) {
            m_provider = m_providerComponent->create();
            if (m_provider) {
                m_provider->setParent(this);
                QQmlEngine::setObjectOwnership(m_provider, QQmlEngine::CppOwnership);
                const auto architecturePanel = m_provider->property("architectureControlPanelComponent");
                const auto singerPanel = m_provider->property("singerControlPanelComponent");
                m_failed = !architecturePanel.canConvert<QQmlComponent *>() || !singerPanel.canConvert<QQmlComponent *>();
                if (!m_failed) {
                    m_architecturePanel = architecturePanel.value<QQmlComponent *>();
                    m_singerPanel = singerPanel.value<QQmlComponent *>();
                }
            } else {
                m_failed = true;
            }
        } else {
            m_failed = true;
        }
        if (m_failed)
            qCWarning(lcControlPanelProvider) << "Could not load control panel provider for architecture" << m_architectureId << m_providerComponent->errors();
        Q_EMIT panelsChanged();
    }

}

#include "moc_ControlPanelProvider.cpp"
