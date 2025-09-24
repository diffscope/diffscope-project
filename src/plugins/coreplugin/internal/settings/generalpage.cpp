#include "generalpage.h"

#include <QApplication>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTranslator>
#include <QLoggingCategory>

#include <SVSCraftQuick/MessageBox.h>

#include <CoreApi/runtimeInterface.h>
#include <CoreApi/translationmanager.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/internal/behaviorpreference.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcGeneralPage, "diffscope.core.generalpage")

    GeneralPage::GeneralPage(QObject *parent) : ISettingPage("core.General", parent) {
        setTitle(tr("General"));
        setDescription(tr("Configure general behaviors of %1").arg(QApplication::applicationName()));

    }
    GeneralPage::~GeneralPage() {
        delete m_widget;
    }
    bool GeneralPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }
    QString GeneralPage::sortKeyword() const {
        return QStringLiteral("General");
    }
    QObject *GeneralPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcGeneralPage) << "Creating widget";
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "GeneralPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }
    void GeneralPage::beginSetting() {
        qCInfo(lcGeneralPage) << "Beginning setting";
        widget();
        m_widget->setProperty("startupBehavior", BehaviorPreference::instance()->property("startupBehavior"));
        qCDebug(lcGeneralPage) << "startupBehavior" << m_widget->property("startupBehavior");
        m_widget->setProperty("useSystemLanguage", BehaviorPreference::instance()->property("useSystemLanguage"));
        qCDebug(lcGeneralPage) << "useSystemLanguage" << m_widget->property("useSystemLanguage");
        m_widget->setProperty("localeName", BehaviorPreference::instance()->property("localeName"));
        qCDebug(lcGeneralPage) << "localeName" << m_widget->property("localeName");
        m_widget->setProperty("hasNotificationSoundAlert", BehaviorPreference::instance()->property("hasNotificationSoundAlert"));
        qCDebug(lcGeneralPage) << "hasNotificationSoundAlert" << m_widget->property("hasNotificationSoundAlert");
        m_widget->setProperty("notificationAutoHideTimeout", BehaviorPreference::instance()->property("notificationAutoHideTimeout"));
        qCDebug(lcGeneralPage) << "notificationAutoHideTimeout" << m_widget->property("notificationAutoHideTimeout");
        m_widget->setProperty("commandPaletteHistoryCount", BehaviorPreference::instance()->property("commandPaletteHistoryCount"));
        qCDebug(lcGeneralPage) << "commandPaletteHistoryCount" << m_widget->property("commandPaletteHistoryCount");
        m_widget->setProperty("proxyOption", BehaviorPreference::instance()->property("proxyOption"));
        qCDebug(lcGeneralPage) << "proxyOption" << m_widget->property("proxyOption");
        m_widget->setProperty("proxyType", BehaviorPreference::instance()->property("proxyType"));
        qCDebug(lcGeneralPage) << "proxyType" << m_widget->property("proxyType");
        m_widget->setProperty("proxyHostname", BehaviorPreference::instance()->property("proxyHostname"));
        qCDebug(lcGeneralPage) << "proxyHostname" << m_widget->property("proxyHostname");
        m_widget->setProperty("proxyPort", BehaviorPreference::instance()->property("proxyPort"));
        qCDebug(lcGeneralPage) << "proxyPort" << m_widget->property("proxyPort");
        m_widget->setProperty("proxyHasAuthentication", BehaviorPreference::instance()->property("proxyHasAuthentication"));
        qCDebug(lcGeneralPage) << "proxyHasAuthentication" << m_widget->property("proxyHasAuthentication");
        m_widget->setProperty("proxyUsername", BehaviorPreference::instance()->property("proxyUsername"));
        qCDebug(lcGeneralPage) << "proxyUsername" << m_widget->property("proxyUsername");
        m_widget->setProperty("proxyPassword", BehaviorPreference::instance()->property("proxyPassword"));
        qCDebug(lcGeneralPage) << "proxyPassword" << m_widget->property("proxyPassword");
        m_widget->setProperty("autoCheckForUpdates", BehaviorPreference::instance()->property("autoCheckForUpdates"));
        qCDebug(lcGeneralPage) << "autoCheckForUpdates" << m_widget->property("autoCheckForUpdates");
        m_widget->setProperty("updateOption", BehaviorPreference::instance()->property("updateOption"));
        qCDebug(lcGeneralPage) << "updateOption" << m_widget->property("updateOption");
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool GeneralPage::accept() {
        qCInfo(lcGeneralPage) << "Accepting";
        bool promptRestartForLanguage = (m_widget->property("localeName").toString() != QLocale().name());
        qCDebug(lcGeneralPage) << "startupBehavior" << m_widget->property("startupBehavior");
        BehaviorPreference::instance()->setProperty("startupBehavior", m_widget->property("startupBehavior"));
        qCDebug(lcGeneralPage) << "useSystemLanguage" << m_widget->property("useSystemLanguage");
        BehaviorPreference::instance()->setProperty("useSystemLanguage", m_widget->property("useSystemLanguage"));
        qCDebug(lcGeneralPage) << "localeName" << m_widget->property("localeName");
        BehaviorPreference::instance()->setProperty("localeName", m_widget->property("localeName"));
        qCDebug(lcGeneralPage) << "hasNotificationSoundAlert" << m_widget->property("hasNotificationSoundAlert");
        BehaviorPreference::instance()->setProperty("hasNotificationSoundAlert", m_widget->property("hasNotificationSoundAlert"));
        qCDebug(lcGeneralPage) << "notificationAutoHideTimeout" << m_widget->property("notificationAutoHideTimeout");
        BehaviorPreference::instance()->setProperty("notificationAutoHideTimeout", m_widget->property("notificationAutoHideTimeout"));
        qCDebug(lcGeneralPage) << "commandPaletteHistoryCount" << m_widget->property("commandPaletteHistoryCount");
        BehaviorPreference::instance()->setProperty("commandPaletteHistoryCount", m_widget->property("commandPaletteHistoryCount"));
        qCDebug(lcGeneralPage) << "proxyOption" << m_widget->property("proxyOption");
        BehaviorPreference::instance()->setProperty("proxyOption", m_widget->property("proxyOption"));
        qCDebug(lcGeneralPage) << "proxyType" << m_widget->property("proxyType");
        BehaviorPreference::instance()->setProperty("proxyType", m_widget->property("proxyType"));
        qCDebug(lcGeneralPage) << "proxyHostname" << m_widget->property("proxyHostname");
        BehaviorPreference::instance()->setProperty("proxyHostname", m_widget->property("proxyHostname"));
        qCDebug(lcGeneralPage) << "proxyPort" << m_widget->property("proxyPort");
        BehaviorPreference::instance()->setProperty("proxyPort", m_widget->property("proxyPort"));
        qCDebug(lcGeneralPage) << "proxyHasAuthentication" << m_widget->property("proxyHasAuthentication");
        BehaviorPreference::instance()->setProperty("proxyHasAuthentication", m_widget->property("proxyHasAuthentication"));
        qCDebug(lcGeneralPage) << "proxyUsername" << m_widget->property("proxyUsername");
        BehaviorPreference::instance()->setProperty("proxyUsername", m_widget->property("proxyUsername"));
        qCDebug(lcGeneralPage) << "proxyPassword" << m_widget->property("proxyPassword");
        BehaviorPreference::instance()->setProperty("proxyPassword", m_widget->property("proxyPassword"));
        qCDebug(lcGeneralPage) << "autoCheckForUpdates" << m_widget->property("autoCheckForUpdates");
        BehaviorPreference::instance()->setProperty("autoCheckForUpdates", m_widget->property("autoCheckForUpdates"));
        qCDebug(lcGeneralPage) << "updateOption" << m_widget->property("updateOption");
        BehaviorPreference::instance()->setProperty("updateOption", m_widget->property("updateOption"));
        BehaviorPreference::instance()->save();
        if (promptRestartForLanguage) {
            qCInfo(lcGeneralPage) << "Language changed" << m_widget->property("localeName").toString() << QLocale().name();
            auto [title, text] = getRestartMessageInNewLanguage(m_widget->property("localeName").toString());
            if (SVS::MessageBox::question(
                RuntimeInterface::qmlEngine(),
                static_cast<QQuickItem *>(m_widget)->window(),
                title.arg(QApplication::applicationName()),
                text.arg(QApplication::applicationName())
            ) == SVS::SVSCraft::Yes) {
                CoreInterface::restartApplication();
            }
        }
        return ISettingPage::accept();
    }
    void GeneralPage::endSetting() {
        qCInfo(lcGeneralPage) << "Ending setting";
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    QVariantList GeneralPage::languages() {
        static QVariantList ret;
        if (ret.isEmpty()) {
            std::ranges::transform(CoreInterface::translationManager()->locales(), std::back_inserter(ret), [](const QString &localeName) {
                auto locale = QLocale(localeName);
                return QVariantMap {
                    {"text", QStringLiteral("%1 (%2)").arg(locale.nativeLanguageName(), locale.nativeTerritoryName())},
                    {"value", localeName}
                };
            });
        }
        return ret;
    }

    bool GeneralPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

    static QString m_translationsDirPath;

    QPair<QString, QString> GeneralPage::getRestartMessageInNewLanguage(const QString &localeName) {
        auto filePath = m_translationsDirPath + "/Core_" + localeName + ".qm";
        QTranslator translator;
        if (!translator.load(filePath)) {
            return {tr("Restart %1"), tr("Restart %1 to apply language changes?")};
        } else {
            return {
                translator.translate(staticMetaObject.className(), "Restart %1"),
                translator.translate(staticMetaObject.className(), "Restart %1 to apply language changes?")
            };
        }
    }

    void GeneralPage::setCorePluginTranslationsPath(const QString &path) {
        m_translationsDirPath = path;
    }

}
