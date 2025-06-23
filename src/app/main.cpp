#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtWidgets/QApplication>

#include <extensionsystem/pluginmanager.h>

#include <CkLoader/loaderspec.h>
#include <CoreApi/applicationinfo.h>
#include <CoreApi/iloader.h>

#include <loadapi/initroutine.h>

#include <application_config.h>

#ifdef APPLICATION_ENABLE_BREAKPAD
#  include <QBreakpadHandler.h>
#endif

class MyLoaderConfiguration : public Loader::LoaderSpec {
public:
    MyLoaderConfiguration() {
        single = false;
        allowRoot = false;
        pluginIID = QStringLiteral(APPLICATION_PLUGIN_IID);
        splashConfigPath = Core::ApplicationInfo::shareDir() + QStringLiteral("/config.json");
        pluginPaths << Core::ApplicationInfo::appPluginsDir();
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
        Core::ILoader &loader = *Core::ILoader::instance();
        loader.setSettingsPath(QSettings::UserScope,
                               QStringLiteral("%1/%2.settings.json")
                                   .arg(userSettingsPath, QStringLiteral(APPLICATION_NAME)));
        loader.setSettingsPath(QSettings::SystemScope,
                               QStringLiteral("%1/%2.settings.json")
                                   .arg(systemSettingsPath, QStringLiteral(APPLICATION_NAME)));
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
    a.setApplicationName(QStringLiteral(APPLICATION_NAME));
    a.setApplicationVersion(QStringLiteral(APPLICATION_VERSION));
    a.setOrganizationName(QStringLiteral(APPLICATION_ORG_NAME));
    a.setOrganizationDomain(QStringLiteral(APPLICATION_ORG_DOMAIN));

    MyLoaderConfiguration loader;

#ifdef APPLICATION_ENABLE_BREAKPAD
    QBreakpadHandler breakpad;
    breakpad.setDumpPath(loader.userSettingsPath + QStringLiteral("/crashes"));
#endif

    return MyLoaderConfiguration().run();
}