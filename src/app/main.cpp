#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QLoggingCategory>
#include <QtGui/QWindow>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleFactory>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuickControls2/QQuickStyle>

#include <CkLoader/loaderspec.h>
#include <CoreApi/applicationinfo.h>
#include <CoreApi/runtimeInterface.h>
#include <CoreApi/logger.h>

#include <qjsonsettings.h>

#include <loadapi/initroutine.h>

#include <application_config.h>

#ifdef APPLICATION_ENABLE_BREAKPAD
#  include <QBreakpadHandler.h>
#endif

using Core::ApplicationInfo;

static QSettings::Format getJsonSettingsFormat() {
    static auto format = QJsonSettings::registerFormat();
    return format;
}

static QQmlEngine *engine{};

class MyLoaderSpec : public Loader::LoaderSpec {
public:
    MyLoaderSpec() {
        userSettingsPath = ApplicationInfo::applicationLocation(ApplicationInfo::RuntimeData);
        systemSettingsPath =
            ApplicationInfo::applicationLocation(ApplicationInfo::BuiltinResources);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, userSettingsPath);
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, systemSettingsPath);

        single = true;
        allowRoot = false;
        pluginIID = QStringLiteral(APPLICATION_PLUGIN_IID);
        splashConfigPath = ApplicationInfo::applicationLocation(ApplicationInfo::BuiltinResources) +
                           QStringLiteral("/config.json");
        pluginPaths << ApplicationInfo::applicationLocation(ApplicationInfo::BuiltinPlugins);
    }

    QSettings *createExtensionSystemSettings(QSettings::Scope scope) override {
        const QString &dir = scope == QSettings::UserScope ? userSettingsPath : systemSettingsPath;
        return new QSettings(QStringLiteral("%1/%2.extensionsystem.ini")
                                 .arg(dir, QCoreApplication::applicationName()),
                             QSettings::IniFormat);
    }

    QSettings *createChorusKitSettings(QSettings::Scope scope) override {
        const QString &dir = scope == QSettings::UserScope ? userSettingsPath : systemSettingsPath;
        return new QSettings(
            QStringLiteral("%1/%2.plugins.json").arg(dir, QCoreApplication::applicationName()),
            getJsonSettingsFormat());
    }

    void splashWillShow(QSplashScreen *screen) override {
        Q_UNUSED(screen);
        // Do nothing
    }

    void beforeLoadPlugins() override {
        // Restore language and themes
        // Core::InitRoutine::initializeAppearance(ExtensionSystem::PluginManager::settings());
        Core::RuntimeInterface::setQmlEngine(engine);
        auto settings = Core::RuntimeInterface::settings();
        if (settings->value("lastInitializationAbortedFlag").toBool()) {
            qInfo() << "Last initialization was aborted abnormally";
            QQmlComponent component(engine, "DiffScope.UIShell", "InitializationFailureWarningDialog");
            std::unique_ptr<QObject> dialog(component.isError() ? nullptr : component.createWithInitialProperties({
                {"logsPath", Core::Logger::logsLocation()}
            }));
            if (!dialog) {
                qFatal() << "Failed to load InitializationFailureWarningDialog" << component.errorString();
            }
            QEventLoop eventLoop;
            QObject::connect(dialog.get(), SIGNAL(done(QVariant)), &eventLoop, SLOT(quit()));
            auto win = qobject_cast<QWindow *>(dialog.get());
            Q_ASSERT(win);
            win->show();
            eventLoop.exec();
        }
        settings->setValue("lastInitializationAbortedFlag", true);
    }

    void afterLoadPlugins() override {
        // Do nothing
    }

    QString userSettingsPath;
    QString systemSettingsPath;
};

#ifdef APPLICATION_ENABLE_BREAKPAD
static void crashHandler() {
    // TODO: execute another process to report the crash

#  ifdef Q_OS_WINDOWS
    ApplicationInfo::messageBox(
        nullptr, ApplicationInfo::Critical, QStringLiteral(APPLICATION_NAME),
        QStringLiteral(
            "This application will exit immediately because an unrecoverable error has occurred."));
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

    QQmlEngine m_engine;
    engine = &m_engine;

#ifdef APPLICATION_ENABLE_BREAKPAD
    QBreakpadHandler breakpad;
    breakpad.setDumpPath(ApplicationInfo::applicationLocation(ApplicationInfo::RuntimeData) +
                         QStringLiteral("/crashes"));
    breakpad.UniqueExtraHandler = crashHandler;
#endif

    // Make sure we honor the system's proxy settings
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // Don't show plugin manager debug info
    QLoggingCategory::setFilterRules(QLatin1String("qtc.*.debug=false"));

    QQuickStyle::setStyle("SVSCraft.UIComponents");
    QQuickStyle::setFallbackStyle("Basic");
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    return MyLoaderSpec().run();
}
