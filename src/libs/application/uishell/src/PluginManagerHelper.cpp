#include "PluginManagerHelper_p.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/plugincollection.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginspec_p.h>

namespace UIShell {

    PluginSpecHelper::PluginSpecHelper(ExtensionSystem::PluginSpec *pluginSpec, QObject *parent) : QObject(parent), m_pluginSpec(pluginSpec) {
        m_initialIsEnabledBySettings = pluginSpec->isEnabledBySettings();
    }
    PluginSpecHelper::~PluginSpecHelper() = default;
    QString PluginSpecHelper::name() const {
        return m_pluginSpec->name();
    }
    QString PluginSpecHelper::displayName() const {
        return m_pluginSpec->displayName();
    }
    QString PluginSpecHelper::version() const {
        return m_pluginSpec->version();
    }
    QString PluginSpecHelper::compatVersion() const {
        return m_pluginSpec->compatVersion();
    }
    QString PluginSpecHelper::vendor() const {
        return m_pluginSpec->vendor();
    }
    QString PluginSpecHelper::copyright() const {
        return m_pluginSpec->copyright();
    }
    QString PluginSpecHelper::license() const {
        return m_pluginSpec->license();
    }
    QString PluginSpecHelper::description() const {
        return m_pluginSpec->description();
    }
    QString PluginSpecHelper::url() const {
        return m_pluginSpec->url();
    }
    QString PluginSpecHelper::category() const {
        return m_pluginSpec->category();
    }
    bool PluginSpecHelper::isAvailableForHostPlatform() const {
        return m_pluginSpec->isAvailableForHostPlatform();
    }
    bool PluginSpecHelper::isRequired() const {
        return m_pluginSpec->isRequired();
    }
    bool PluginSpecHelper::isEnabledIndirectly() const {
        return m_pluginSpec->isEnabledIndirectly();
    }
    bool PluginSpecHelper::isForceEnabled() const {
        return m_pluginSpec->isForceEnabled();
    }
    bool PluginSpecHelper::isForceDisabled() const {
        return m_pluginSpec->isForceDisabled();
    }
    bool PluginSpecHelper::hasError() const {
        return m_pluginSpec->hasError();
    }
    QString PluginSpecHelper::errorString() const {
        return m_pluginSpec->errorString();
    }
    bool PluginSpecHelper::isEnabledBySettings() const {
        return m_pluginSpec->isEnabledBySettings();
    }
    void PluginSpecHelper::setEnabledBySettings(bool value) {
        auto d = *reinterpret_cast<ExtensionSystem::Internal::PluginSpecPrivate **>(m_pluginSpec);
        if (value != d->enabledBySettings) {
            d->setEnabledBySettings(value);
            emit enabledBySettingsChanged();
            emit restartRequiredChanged();
        }
    }
    bool PluginSpecHelper::isRestartRequired() const {
        return m_pluginSpec->isEnabledBySettings() != m_initialIsEnabledBySettings;
    }
    bool PluginSpecHelper::isRunning() const {
        return m_pluginSpec->state() == ExtensionSystem::PluginSpec::Running;
    }



    PluginCollectionHelper::PluginCollectionHelper(ExtensionSystem::PluginCollection *pluginCollection, PluginManagerHelper *parent) : QObject(parent), m_pluginCollection(pluginCollection) {
    }

    PluginCollectionHelper::~PluginCollectionHelper() = default;

    PluginManagerHelper *PluginCollectionHelper::pluginManagerHelper() const {
        return static_cast<PluginManagerHelper *>(parent());
    }

    QString PluginCollectionHelper::name() const {
        return m_pluginCollection->name();
    }

    QList<PluginSpecHelper *> PluginCollectionHelper::plugins() const {
        QList<PluginSpecHelper *> result;
        for (auto pluginSpec : m_pluginCollection->plugins()) {
            result.append(pluginManagerHelper()->getHelper(pluginSpec));
        }
        return result;
    }



    PluginManagerHelper::PluginManagerHelper(QObject *parent) : QObject(parent) {
    }

    PluginManagerHelper::~PluginManagerHelper() = default;

    QList<PluginCollectionHelper *> PluginManagerHelper::pluginCollections() {
        QList<PluginCollectionHelper *> result;
        for (auto pluginCollection : ExtensionSystem::PluginManager::pluginCollections()) {
            result.append(getHelper(pluginCollection));
        }
        return result;
    }

    PluginCollectionHelper *PluginManagerHelper::getHelper(ExtensionSystem::PluginCollection *pluginCollection) {
        if (m_pluginCollections.contains(pluginCollection)) {
            return m_pluginCollections.value(pluginCollection);
        }
        auto helper = new PluginCollectionHelper(pluginCollection, this);
        m_pluginCollections.insert(pluginCollection, helper);
        return helper;
    }

    PluginSpecHelper *PluginManagerHelper::getHelper(ExtensionSystem::PluginSpec *pluginSpec) {
        if (m_pluginSpecs.contains(pluginSpec)) {
            return m_pluginSpecs.value(pluginSpec);
        }
        auto helper = new PluginSpecHelper(pluginSpec, this);
        m_pluginSpecs.insert(pluginSpec, helper);
        return helper;
    }

}