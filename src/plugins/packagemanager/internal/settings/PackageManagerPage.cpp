#include "PackageManagerPage.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <packagemanager/internal/PackageManagerSettings.h>
#include <packagemanager/internal/PackageManagerUtils.h>

namespace PackageManager {

    Q_STATIC_LOGGING_CATEGORY(lcPackageManagerPage, "diffscope.packagemanager.packagemanagerpage")

    namespace {

        struct DspmValidationResult {
            bool ok{};
            bool compatible{};
            QString name;
            QString version;
            QString errorMessage;
        };

        DspmValidationResult validateDspmExecutable(const QString &path, int timeoutSeconds) {
            DspmValidationResult result;
            const auto commandResult = runCommandLineTool(path, QStringList{QStringLiteral("--version"), QStringLiteral("--json")}, timeoutSeconds);
            if (!commandResult.ok) {
                result.errorMessage = commandResult.errorMessage;
                return result;
            }

            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(commandResult.stdOut, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                result.errorMessage = PackageManagerPage::tr("The command did not return valid JSON.");
                return result;
            }
            const auto root = document.object();
            result.name = root.value(QStringLiteral("name")).toString();
            result.version = root.value(QStringLiteral("version")).toString();
            if (result.name != QStringLiteral("dspm")) {
                result.errorMessage = PackageManagerPage::tr("The selected command is not dspm.");
                return result;
            }

            result.ok = true;
            // TODO: Replace this placeholder with real compatibility rules once supported dspm versions are defined.
            result.compatible = false;
            return result;
        }

        QString urlToLocalFile(const QUrl &url) {
            if (url.isLocalFile()) {
                return url.toLocalFile();
            }
            return url.toString(QUrl::PreferLocalFile);
        }

    }

    PackageManagerPage::PackageManagerPage(QObject *parent)
        : Core::ISettingPage("org.diffscope.packagemanager.PackageManager", parent) {
        setTitle(tr("Package Manager"));
        setDescription(tr("Configure package manager settings"));
    }

    PackageManagerPage::~PackageManagerPage() {
        delete m_widget;
    }

    bool PackageManagerPage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QString PackageManagerPage::sortKeyword() const {
        return QStringLiteral("Package Manager");
    }

    QObject *PackageManagerPage::widget() {
        if (m_widget)
            return m_widget;

        qCDebug(lcPackageManagerPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.PackageManager", "PackageManagerPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void PackageManagerPage::beginSetting() {
        qCInfo(lcPackageManagerPage) << "Beginning setting";
        widget();
        m_widget->setProperty("dspmPath", PackageManagerSettings::instance()->property("dspmPath"));
        m_widget->setProperty("packageDir", PackageManagerSettings::instance()->property("packageDir"));
        m_widget->setProperty("timeoutSeconds", PackageManagerSettings::instance()->property("timeoutSeconds"));
        m_widget->setProperty("started", true);
        Core::ISettingPage::beginSetting();
    }

    bool PackageManagerPage::accept() {
        qCInfo(lcPackageManagerPage) << "Accepting";
        PackageManagerSettings::instance()->setProperty("dspmPath", m_widget->property("dspmPath"));
        PackageManagerSettings::instance()->setProperty("packageDir", m_widget->property("packageDir"));
        PackageManagerSettings::instance()->setProperty("timeoutSeconds", m_widget->property("timeoutSeconds"));
        PackageManagerSettings::instance()->save();
        return Core::ISettingPage::accept();
    }

    void PackageManagerPage::endSetting() {
        qCInfo(lcPackageManagerPage) << "Ending setting";
        m_widget->setProperty("started", false);
        Core::ISettingPage::endSetting();
    }

    bool PackageManagerPage::validateDspmPath(const QUrl &url, int timeoutSeconds) {
        const auto path = urlToLocalFile(url);
        qCInfo(lcPackageManagerPage) << "Validating dspm path" << path;
        const auto result = validateDspmExecutable(path, timeoutSeconds);
        if (!result.ok) {
            qCWarning(lcPackageManagerPage) << "Failed to validate dspm path" << path << result.errorMessage;
            SVS::MessageBox::critical(
                Core::RuntimeInterface::qmlEngine(),
                nullptr,
                tr("Invalid Package Manager"),
                tr("Failed to validate the selected command:\n\n%1").arg(result.errorMessage)
            );
            return false;
        }

        if (!result.compatible) {
            qCWarning(lcPackageManagerPage) << "Incompatible dspm version" << result.version;
            SVS::MessageBox::warning(
                Core::RuntimeInterface::qmlEngine(),
                nullptr,
                tr("Incompatible Package Manager"),
                tr("The selected dspm version (%1) is not marked as compatible. It will still be used.").arg(result.version)
            );
        }
        return true;
    }

    QString PackageManagerPage::localFilePath(const QUrl &url) {
        return urlToLocalFile(url);
    }

    bool PackageManagerPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
