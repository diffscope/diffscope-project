#include "generalpage.h"

#include <QApplication>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>

#include <SVSCraftQuick/MessageBox.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/behaviorpreference.h>

namespace Core::Internal {
    GeneralPage::GeneralPage(QObject *parent) : ISettingPage("core.General", parent) {
        setTitle(tr("General"));
        setDescription(tr("Configure general behaviors of %1").arg(QApplication::applicationName()));

    }
    GeneralPage::~GeneralPage() {
        delete m_widget;
    }
    bool GeneralPage::matches(const QString &word) const {
        return ISettingPage::matches(word);
    }
    QString GeneralPage::sortKeyword() const {
        return QStringLiteral("General");
    }
    QObject *GeneralPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "GeneralPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }
    void GeneralPage::beginSetting() {
        widget();
        m_widget->setProperty("startupBehavior", ICore::behaviorPreference()->property("startupBehavior"));
        m_widget->setProperty("useSystemLanguage", ICore::behaviorPreference()->property("useSystemLanguage"));
        m_widget->setProperty("localeName", ICore::behaviorPreference()->property("localeName"));
        m_widget->setProperty("hasNotificationSoundAlert", ICore::behaviorPreference()->property("hasNotificationSoundAlert"));
        m_widget->setProperty("notificationAutoHideTimeout", ICore::behaviorPreference()->property("notificationAutoHideTimeout"));
        m_widget->setProperty("proxyOption", ICore::behaviorPreference()->property("proxyOption"));
        m_widget->setProperty("ProxyType", ICore::behaviorPreference()->property("ProxyType"));
        m_widget->setProperty("proxyHostname", ICore::behaviorPreference()->property("proxyHostname"));
        m_widget->setProperty("proxyPort", ICore::behaviorPreference()->property("proxyPort"));
        m_widget->setProperty("proxyHasAuthentication", ICore::behaviorPreference()->property("proxyHasAuthentication"));
        m_widget->setProperty("proxyUsername", ICore::behaviorPreference()->property("proxyUsername"));
        m_widget->setProperty("proxyPassword", ICore::behaviorPreference()->property("proxyPassword"));
        m_widget->setProperty("autoCheckForUpdates", ICore::behaviorPreference()->property("autoCheckForUpdates"));
        m_widget->setProperty("updateOption", ICore::behaviorPreference()->property("updateOption"));
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }
    bool GeneralPage::accept() {
        Q_ASSERT(m_widget);
        bool promptRestartForLanguage = (m_widget->property("localeName").toString() != ICore::behaviorPreference()->localeName());
        ICore::behaviorPreference()->setProperty("startupBehavior", m_widget->property("startupBehavior"));
        ICore::behaviorPreference()->setProperty("useSystemLanguage", m_widget->property("useSystemLanguage"));
        ICore::behaviorPreference()->setProperty("localeName", m_widget->property("localeName"));
        ICore::behaviorPreference()->setProperty("hasNotificationSoundAlert", m_widget->property("hasNotificationSoundAlert"));
        ICore::behaviorPreference()->setProperty("notificationAutoHideTimeout", m_widget->property("notificationAutoHideTimeout"));
        ICore::behaviorPreference()->setProperty("proxyOption", m_widget->property("proxyOption"));
        ICore::behaviorPreference()->setProperty("ProxyType", m_widget->property("ProxyType"));
        ICore::behaviorPreference()->setProperty("proxyHostname", m_widget->property("proxyHostname"));
        ICore::behaviorPreference()->setProperty("proxyPort", m_widget->property("proxyPort"));
        ICore::behaviorPreference()->setProperty("proxyHasAuthentication", m_widget->property("proxyHasAuthentication"));
        ICore::behaviorPreference()->setProperty("proxyUsername", m_widget->property("proxyUsername"));
        ICore::behaviorPreference()->setProperty("proxyPassword", m_widget->property("proxyPassword"));
        ICore::behaviorPreference()->setProperty("autoCheckForUpdates", m_widget->property("autoCheckForUpdates"));
        ICore::behaviorPreference()->setProperty("updateOption", m_widget->property("updateOption"));
        ICore::behaviorPreference()->save();
        if (true || promptRestartForLanguage) {
            if (SVS::MessageBox::question(
                PluginDatabase::qmlEngine(),
                static_cast<QQuickItem *>(m_widget)->window(),
                tr("Restart %1").arg(QApplication::applicationName()),
                tr("Restart %1 to apply language changes?")
            ) == SVS::SVSCraft::Yes) {
                ICore::restartApplication();
            }
        }
        return ISettingPage::accept();
    }
    void GeneralPage::endSetting() {
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

}
