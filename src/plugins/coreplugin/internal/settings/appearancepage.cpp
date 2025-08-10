#include "appearancepage.h"

#include <QApplication>
#include <QQmlComponent>
#include <QFontDatabase>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/behaviorpreference.h>

namespace Core::Internal {
    AppearancePage::AppearancePage(QObject *parent) : ISettingPage("core.Appearance", parent) {
        setTitle(tr("Appearance"));
        setDescription(tr("Configure how %1 looks like").arg(QApplication::applicationName()));
    }
    AppearancePage::~AppearancePage() {
        delete m_widget;
    }
    QString AppearancePage::sortKeyword() const {
        return QStringLiteral("Appearance");
    }
    bool AppearancePage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }
    QObject *AppearancePage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "AppearancePage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }
    void AppearancePage::beginSetting() {
        widget();
        m_widget->setProperty("useCustomFont", ICore::behaviorPreference()->property("useCustomFont"));
        m_widget->setProperty("fontFamily", ICore::behaviorPreference()->property("fontFamily"));
        m_widget->setProperty("fontStyle", ICore::behaviorPreference()->property("fontStyle"));
        m_widget->setProperty("uiBehavior", ICore::behaviorPreference()->property("uiBehavior"));
        m_widget->setProperty("graphicsBehavior", ICore::behaviorPreference()->property("graphicsBehavior"));
        m_widget->setProperty("animationEnabled", ICore::behaviorPreference()->property("animationEnabled"));
        m_widget->setProperty("animationSpeedRatio", ICore::behaviorPreference()->property("animationSpeedRatio"));
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }
    bool AppearancePage::accept() {
        ICore::behaviorPreference()->setProperty("useCustomFont", m_widget->property("useCustomFont"));
        ICore::behaviorPreference()->setProperty("fontFamily", m_widget->property("fontFamily"));
        ICore::behaviorPreference()->setProperty("fontStyle", m_widget->property("fontStyle"));
        ICore::behaviorPreference()->setProperty("uiBehavior", m_widget->property("uiBehavior"));
        ICore::behaviorPreference()->setProperty("graphicsBehavior", m_widget->property("graphicsBehavior"));
        ICore::behaviorPreference()->setProperty("animationEnabled", m_widget->property("animationEnabled"));
        ICore::behaviorPreference()->setProperty("animationSpeedRatio", m_widget->property("animationSpeedRatio"));
        ICore::behaviorPreference()->save();
        return ISettingPage::accept();
    }
    void AppearancePage::endSetting() {
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }
    QStringList AppearancePage::fontFamilies() {
        return QFontDatabase::families();
    }
    QStringList AppearancePage::fontStyles(const QString &family) {
        return QFontDatabase::styles(family);
    }
    bool AppearancePage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }
}