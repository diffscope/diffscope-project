#include "appearancepage.h"

#include <QApplication>
#include <QQmlComponent>
#include <QFontDatabase>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/internal/behaviorpreference.h>

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
        m_widget->setProperty("useCustomFont", BehaviorPreference::instance()->property("useCustomFont"));
        m_widget->setProperty("fontFamily", BehaviorPreference::instance()->property("fontFamily"));
        m_widget->setProperty("fontStyle", BehaviorPreference::instance()->property("fontStyle"));
        m_widget->setProperty("uiBehavior", BehaviorPreference::instance()->property("uiBehavior"));
        m_widget->setProperty("graphicsBehavior", BehaviorPreference::instance()->property("graphicsBehavior"));
        m_widget->setProperty("animationEnabled", BehaviorPreference::instance()->property("animationEnabled"));
        m_widget->setProperty("animationSpeedRatio", BehaviorPreference::instance()->property("animationSpeedRatio"));
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }
    bool AppearancePage::accept() {
        BehaviorPreference::instance()->setProperty("useCustomFont", m_widget->property("useCustomFont"));
        BehaviorPreference::instance()->setProperty("fontFamily", m_widget->property("fontFamily"));
        BehaviorPreference::instance()->setProperty("fontStyle", m_widget->property("fontStyle"));
        BehaviorPreference::instance()->setProperty("uiBehavior", m_widget->property("uiBehavior"));
        BehaviorPreference::instance()->setProperty("graphicsBehavior", m_widget->property("graphicsBehavior"));
        BehaviorPreference::instance()->setProperty("animationEnabled", m_widget->property("animationEnabled"));
        BehaviorPreference::instance()->setProperty("animationSpeedRatio", m_widget->property("animationSpeedRatio"));
        BehaviorPreference::instance()->save();
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