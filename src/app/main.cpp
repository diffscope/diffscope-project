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
#include <CoreApi/runtimeinterface.h>
#include <CoreApi/logger.h>
#include <CoreApi/translationmanager.h>

#include <qjsonsettings.h>

#include <loadapi/initroutine.h>

#include <application_config.h>
#include <application_buildinfo.h>

#ifdef APPLICATION_ENABLE_BREAKPAD
#  include <QBreakpadHandler.h>
#endif

using namespace Core;

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
        coreName = QStringLiteral("org.diffscope.core");
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

    void splashShown(QSplashScreen *screen) override {
        QMetaObject::invokeMethod(screen, "setText", QStringLiteral("appVersion"), QApplication::translate("Application", "Version %1").arg(APPLICATION_SEMVER));
        QMetaObject::invokeMethod(screen, "setText", QStringLiteral("copyright"), QApplication::translate("Application", "Copyright \u00a9 %1-%2 %3. All rights reserved.").arg(
            QLocale().toString(QDate(QStringLiteral(APPLICATION_DEV_START_YEAR).toInt(), 1, 1), "yyyy"),
            QLocale().toString(QDate(QStringLiteral(APPLICATION_BUILD_YEAR).toInt(), 1, 1), "yyyy"),
            APPLICATION_VENDOR_NAME
        ));
    }

    void beforeLoadPlugins() override {
        RuntimeInterface::setQmlEngine(engine);
        auto settings = RuntimeInterface::settings();

        QLocale locale;

        settings->beginGroup("Core::Internal::BehaviorPreference");
        if (settings->value("useSystemLanguage", true).toBool()) {
            locale = QLocale::system();
        } else {
            locale = QLocale(settings->value("localeName", QLocale().name()).toString());
        }
        settings->endGroup();
        locale.setNumberOptions(QLocale::OmitGroupSeparator);
        RuntimeInterface::setTranslationManager(new TranslationManager(RuntimeInterface::instance()));
        RuntimeInterface::translationManager()->setLocale(locale);
        RuntimeInterface::translationManager()->addTranslationPath(ApplicationInfo::systemLocation(ApplicationInfo::Resources) + QStringLiteral("/ChorusKit/translations"));
        RuntimeInterface::translationManager()->addTranslationPath(ApplicationInfo::systemLocation(ApplicationInfo::Resources) + QStringLiteral("/svscraft/translations"));
        RuntimeInterface::translationManager()->addTranslationPath(ApplicationInfo::systemLocation(ApplicationInfo::Resources) + QStringLiteral("/uishell/translations"));

        if (settings->value("lastInitializationAbortedFlag").toBool()) {
            qInfo() << "Last initialization was aborted abnormally";
            QQmlComponent component(engine, "DiffScope.UIShell", "InitializationFailureWarningDialog");
            std::unique_ptr<QObject> dialog(component.isError() ? nullptr : component.createWithInitialProperties({
                {"logsPath", Logger::logsLocation()}
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
    a.setApplicationDisplayName(QStringLiteral(APPLICATION_DISPLAY_NAME));
    a.setApplicationVersion(QStringLiteral(APPLICATION_SEMVER));
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
