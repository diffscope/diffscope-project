#include "MaintenanceAddOn.h"

#include <application_config.h>

#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/logger.h>
#include <CoreApi/runtimeinterface.h>

#include <SVSCraftGui/DesktopServices.h>
#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ActionWindowInterfaceBase.h>

#include <maintenance/internal/maintenanceplugin.h>

namespace Maintenance {

    Q_STATIC_LOGGING_CATEGORY(lcMaintenanceAddOn, "diffscope.maintenance.maintenanceaddon")

    MaintenanceAddOn::MaintenanceAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    MaintenanceAddOn::~MaintenanceAddOn() = default;

    static QPointer<QQuickWindow> m_window;
    static MaintenancePlugin *m_plugin = nullptr;

    void MaintenanceAddOn::setPlugin(MaintenancePlugin *plugin) {
        m_plugin = plugin;
    }

    void MaintenanceAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Maintenance", "MaintenanceActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void MaintenanceAddOn::extensionsInitialized() {
    }

    bool MaintenanceAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    static const QUrl ISSUE_URL("https://github.com/diffscope/diffscope-project/issues");
    static const QUrl CONTRIBUTE_URL("https://diffscope.org/contribute");
    static const QUrl COMMUNITY_URL("https://diffscope.org/community");
    static const QUrl RELEASE_LOG_URL("https://diffscope.org/releases/" APPLICATION_SEMVER);

    void MaintenanceAddOn::reveal(RevealFlag flag) {
        qCInfo(lcMaintenanceAddOn) << "Reveal" << flag;
        switch (flag) {
            case Logs:
                SVS::DesktopServices::reveal(Core::Logger::logsLocation());
                break;
            case Data:
                SVS::DesktopServices::reveal(Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData));
                break;
            case Plugins:
                SVS::DesktopServices::reveal(Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::BuiltinPlugins));
                break;
            case Issue:
                QDesktopServices::openUrl(ISSUE_URL);
                break;
            case Contribute:
                QDesktopServices::openUrl(CONTRIBUTE_URL);
                break;
            case Community:
                QDesktopServices::openUrl(COMMUNITY_URL);
                break;
            case ReleaseLog:
                QDesktopServices::openUrl(RELEASE_LOG_URL);
                break;
        }
    }

    void MaintenanceAddOn::generateDiagnosticReport() const {
        qCInfo(lcMaintenanceAddOn) << "Generate diagnostic report";
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        SVS::MessageBox::information(
            Core::RuntimeInterface::qmlEngine(),
            windowInterface->window(),
            tr("Important Notice"),
            tr(
                "<p>The diagnostic report may contain <b>sensitive information</b> (e.g., usernames, hostnames, network addresses, personalized configurations).</p>\n"
                "<p>If you do not wish to make such information public, please <b>do not</b> share this report on public platforms like GitHub Issues or online chat rooms.</p>\n"
                "<p>You may follow the feedback guidelines to send the report privately via email to the DiffScope development team.</p>\n"
                "<p>Please note that the DiffScope development team <b>does not guarantee</b> that the report will not be disclosed to third parties. It is your decision whether to send the report.</p>\n"
            )
        );
        auto reportId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        auto path = QFileDialog::getSaveFileName(windowInterface->invisibleCentralWidget(), tr("Save Diagnostic Report"), reportId + ".json", "*.json");
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(lcMaintenanceAddOn) << "Failed to open the file for writing:" << path;
            SVS::MessageBox::warning(
                Core::RuntimeInterface::qmlEngine(),
                windowInterface->window(),
                tr("Error"),
                tr("Failed to open the file for writing:\n\n%1").arg(path)
            );
            return;
        }
        QJsonObject report;
        report.insert("$id", reportId);
        auto objects = Core::RuntimeInterface::instance()->getObjects("org.diffscope.maintenance.diagnosisproviders");
        for (auto object : objects) {
            auto key = object->objectName();
            QJsonValue value;
            if (!QMetaObject::invokeMethod(object, "generate", Q_RETURN_ARG(QJsonValue, value))) {
                qCWarning(lcMaintenanceAddOn) << "Failed to invoke `QJsonValue generate()` on" << object;
                continue;
            }
            report.insert(key, value);
        }
        file.write(QJsonDocument(report).toJson());
        QQmlComponent textFieldComponent(Core::RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "TextField");
        if (textFieldComponent.isError()) {
            qFatal() << textFieldComponent.errorString();
        }
        std::unique_ptr<QObject> textField(textFieldComponent.createWithInitialProperties({
            {"readOnly", true},
            {"text", reportId},
        }));
        QQmlComponent dialogComponent(Core::RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        if (dialogComponent.isError()) {
            qFatal() << dialogComponent.errorString();
        }
        std::unique_ptr<QObject> dialog(dialogComponent.createWithInitialProperties({
            {"text", tr("Diagnostic Report Generated")},
            {"informativeText", tr("The identifier of report is:")},
            {"icon", SVS::SVSCraft::Success},
            {"content", QVariant::fromValue(textField.get())},
            {"transientParent", QVariant::fromValue(windowInterface->window())},
        }));
        SVS::MessageBox::customExec(dialog.get());
    }

}
