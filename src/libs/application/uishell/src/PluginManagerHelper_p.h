#ifndef UISHELL_PLUGINMANAGERHELPER_H
#define UISHELL_PLUGINMANAGERHELPER_H

#include <QObject>
#include <QHash>

namespace ExtensionSystem {
    class PluginSpec;
    class PluginCollection;
}

namespace UIShell {

    class PluginManagerHelper;

    class PluginSpecHelper : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(QString displayName READ displayName CONSTANT)
        Q_PROPERTY(QString version READ version CONSTANT)
        Q_PROPERTY(QString compatVersion READ compatVersion CONSTANT)
        Q_PROPERTY(QString vendor READ vendor CONSTANT)
        Q_PROPERTY(QString copyright READ copyright CONSTANT)
        Q_PROPERTY(QString license READ license CONSTANT)
        Q_PROPERTY(QString description READ description CONSTANT)
        Q_PROPERTY(QString url READ url CONSTANT)
        Q_PROPERTY(QString category READ category CONSTANT)
        Q_PROPERTY(bool availableForHostPlatform READ isAvailableForHostPlatform CONSTANT)
        Q_PROPERTY(bool required READ isRequired CONSTANT)
        Q_PROPERTY(bool enabledIndirectly READ isEnabledIndirectly CONSTANT)
        Q_PROPERTY(bool forceEnabled READ isForceEnabled CONSTANT)
        Q_PROPERTY(bool forceDisabled READ isForceDisabled CONSTANT)
        Q_PROPERTY(bool hasError READ hasError CONSTANT)
        Q_PROPERTY(QString errorString READ errorString CONSTANT)
        Q_PROPERTY(bool enabledBySettings READ isEnabledBySettings WRITE setEnabledBySettings NOTIFY enabledBySettingsChanged)
        Q_PROPERTY(bool restartRequired READ isRestartRequired NOTIFY restartRequiredChanged)
        Q_PROPERTY(bool running READ isRunning CONSTANT)
    public:
        explicit PluginSpecHelper(ExtensionSystem::PluginSpec *pluginSpec, QObject *parent = nullptr);
        ~PluginSpecHelper() override;

        QString name() const;
        QString displayName() const;
        QString version() const;
        QString compatVersion() const;
        QString vendor() const;
        QString copyright() const;
        QString license() const;
        QString description() const;
        QString url() const;
        QString category() const;
        bool isAvailableForHostPlatform() const;
        bool isRequired() const;
        bool isEnabledIndirectly() const;
        bool isForceEnabled() const;
        bool isForceDisabled() const;
        bool hasError() const;
        QString errorString() const;

        bool isEnabledBySettings() const;
        void setEnabledBySettings(bool value);

        bool isRestartRequired() const;
        bool isRunning() const;

    signals:
        void enabledBySettingsChanged();
        void restartRequiredChanged();

    private:
        ExtensionSystem::PluginSpec *m_pluginSpec;
        bool m_initialIsEnabledBySettings;
    };

    class PluginCollectionHelper : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(QList<PluginSpecHelper *> plugins READ plugins CONSTANT)
    public:
        explicit PluginCollectionHelper(ExtensionSystem::PluginCollection *pluginCollection, PluginManagerHelper *parent = nullptr);
        ~PluginCollectionHelper() override;

        PluginManagerHelper *pluginManagerHelper() const;

        QString name() const;
        QList<PluginSpecHelper *> plugins() const;

    private:
        ExtensionSystem::PluginCollection *m_pluginCollection;
    };

    class PluginManagerHelper : public QObject {
        Q_OBJECT
        Q_PROPERTY(QList<PluginCollectionHelper *> pluginCollections READ pluginCollections CONSTANT)
    public:
        explicit PluginManagerHelper(QObject *parent = nullptr);
        ~PluginManagerHelper() override;

        QList<PluginCollectionHelper *> pluginCollections();

        PluginCollectionHelper *getHelper(ExtensionSystem::PluginCollection *pluginCollection);
        PluginSpecHelper *getHelper(ExtensionSystem::PluginSpec *pluginSpec);

    private:
        QHash<ExtensionSystem::PluginCollection *, PluginCollectionHelper *> m_pluginCollections;
        QHash<ExtensionSystem::PluginSpec *, PluginSpecHelper *> m_pluginSpecs;
    };

}

#endif //UISHELL_PLUGINMANAGERHELPER_H
