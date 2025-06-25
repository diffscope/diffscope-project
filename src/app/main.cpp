#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtWidgets/QApplication>

#include <extensionsystem/pluginmanager.h>

#include <CkLoader/loaderspec.h>
#include <CoreApi/applicationinfo.h>

#include <loadapi/initroutine.h>

#include <application_config.h>

#ifdef APPLICATION_ENABLE_BREAKPAD
#  include <QBreakpadHandler.h>
#endif

using Core::ApplicationInfo;

class MyLoaderSpec : public Loader::LoaderSpec {
public:
    MyLoaderSpec() {
        single = false;
        allowRoot = false;
        pluginIID = QStringLiteral(APPLICATION_PLUGIN_IID);
        splashConfigPath = ApplicationInfo::applicationLocation(ApplicationInfo::BuiltinResources) +
                           QStringLiteral("/config.json");
        pluginPaths << ApplicationInfo::applicationLocation(ApplicationInfo::BuiltinPlugins);
    }

    void splashWillShow(QSplashScreen *screen) override {
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
    }

    void afterLoadPlugins() override {
        // Do nothing
    }

    QSplashScreen *splash = nullptr;
};

#ifdef APPLICATION_ENABLE_BREAKPAD
static void crashHandler() {
    // TODO: execute another process to report the crash

#  ifdef Q_OS_WINDOWS
    ApplicationInfo::messageBox(
        nullptr, ApplicationInfo::Critical, QStringLiteral("Error"),
        QStringLiteral("An unrecoverable error occurred, this application will now terminate."));
#  endif
}
#endif

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

#ifdef APPLICATION_ENABLE_BREAKPAD
    QBreakpadHandler breakpad;
    breakpad.setDumpPath(ApplicationInfo::applicationLocation(ApplicationInfo::RuntimeData) +
                         QStringLiteral("/crashes"));
    breakpad.UniqueExtraHandler = crashHandler;
#endif

    return MyLoaderSpec().run();
}