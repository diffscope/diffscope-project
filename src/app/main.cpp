#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtWidgets/QApplication>

#include <extensionsystem/pluginmanager.h>

#include <CkLoader/loaderspec.h>

#include <loadapi/initroutine.h>

#include <choruskit_config.h>

#ifdef CONFIG_ENABLE_BREAKPAD
#  include <QBreakpadHandler.h>
#endif

static QString shareDir() {
    return QDir::cleanPath(qApp->applicationDirPath() + "/../share/" + qApp->applicationName());
}

static QString pluginsDir() {
    return QDir::cleanPath(qApp->applicationDirPath() + "/../lib/" +
                           QApplication::applicationName() + "/plugins");
}

class MyLoaderConfiguration : public Loader::LoaderSpec {
public:
    MyLoaderConfiguration() {
        single = false;
        allowRoot = false;
        pluginIID = QStringLiteral(APP_PLUGIN_IID);
        splashConfigPath = shareDir() + QStringLiteral("/config.json");

        pluginPaths << pluginsDir();
    }

    void splashWillShow(QSplashScreen *screen) override {
        splash = screen;

        // Don't show plugin manager debug info
        QLoggingCategory::setFilterRules(QLatin1String("qtc.*.debug=false"));
    }

    void beforeLoadPlugins() override {
        // Set global settings path
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, userSettingsPath);
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, systemSettingsPath);

        // Make sure we honor the system's proxy settings
        QNetworkProxyFactory::setUseSystemConfiguration(true);

        // Restore language and themes
        // Core::InitRoutine::initializeAppearance(ExtensionSystem::PluginManager::settings());
        Core::InitRoutine::setSplash(splash);

        // Set ILoader settings path
        // Core::ILoader &loader = *Core::ILoader::instance();
        // loader.setSettingsPath(
        //     QSettings::UserScope,
        //     QStringLiteral("%1/%2.settings.json").arg(userSettingsPath,
        //     QStringLiteral(APP_NAME)));
        // loader.setSettingsPath(QSettings::SystemScope,
        //                        QStringLiteral("%1/%2.settings.json")
        //                            .arg(systemSettingsPath, QStringLiteral(APP_NAME)));
    }

    void afterLoadPlugins() override {
        // Do nothing
    }

    QSplashScreen *splash = nullptr;
};

int main(int argc, char *argv[]) {
    // Make sure Qt uses the plugin path in qt.conf
    if (qEnvironmentVariableIsSet("QT_PLUGIN_PATH")) {
        qputenv("QT_PLUGIN_PATH", {});
    }

    // Frameless window requires it
    QGuiApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral(APP_NAME));
    a.setApplicationVersion(QStringLiteral(APP_VERSION));
    a.setOrganizationName(QStringLiteral(APP_ORG_NAME));
    a.setOrganizationDomain(QStringLiteral(APP_ORG_DOMAIN));

    MyLoaderConfiguration loader;

#ifdef CONFIG_ENABLE_BREAKPAD
    QBreakpadHandler breakpad;
    breakpad.setDumpPath(loader.userSettingsPath + QStringLiteral("/crashes"));
#endif

    return MyLoaderConfiguration().run();
}