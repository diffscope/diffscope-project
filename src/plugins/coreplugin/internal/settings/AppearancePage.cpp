#include "AppearancePage.h"

#include <QApplication>
#include <QFontDatabase>
#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/BehaviorPreference.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAppearancePage, "diffscope.core.appearancepage")

    AppearancePage::AppearancePage(QObject *parent) : ISettingPage("org.diffscope.core.Appearance", parent) {
        setTitle(tr("Appearance"));
        setDescription(tr("Configure how %1 looks like").arg(QApplication::applicationDisplayName()));
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
        qCDebug(lcAppearancePage) << "Creating widget";
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "AppearancePage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }
    void AppearancePage::beginSetting() {
        qCInfo(lcAppearancePage) << "Beginning setting";
        widget();
        m_widget->setProperty("useCustomFont", BehaviorPreference::instance()->property("useCustomFont"));
        qCDebug(lcAppearancePage) << m_widget->property("useCustomFont");
        m_widget->setProperty("fontFamily", BehaviorPreference::instance()->property("fontFamily"));
        qCDebug(lcAppearancePage) << m_widget->property("fontFamily");
        m_widget->setProperty("fontStyle", BehaviorPreference::instance()->property("fontStyle"));
        qCDebug(lcAppearancePage) << m_widget->property("fontStyle");
        m_widget->setProperty("uiBehavior", BehaviorPreference::instance()->property("uiBehavior"));
        qCDebug(lcAppearancePage) << m_widget->property("uiBehavior");
        m_widget->setProperty("graphicsBehavior", BehaviorPreference::instance()->property("graphicsBehavior"));
        qCDebug(lcAppearancePage) << m_widget->property("graphicsBehavior");
        m_widget->setProperty("animationEnabled", BehaviorPreference::instance()->property("animationEnabled"));
        qCDebug(lcAppearancePage) << m_widget->property("animationEnabled");
        m_widget->setProperty("animationSpeedRatio", BehaviorPreference::instance()->property("animationSpeedRatio"));
        qCDebug(lcAppearancePage) << m_widget->property("animationSpeedRatio");
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }
    bool AppearancePage::accept() {
        qCInfo(lcAppearancePage) << "Accepting";
        qCDebug(lcAppearancePage) << "useCustomFont" << m_widget->property("useCustomFont");
        BehaviorPreference::instance()->setProperty("useCustomFont", m_widget->property("useCustomFont"));
        qCDebug(lcAppearancePage) << "fontFamily" << m_widget->property("fontFamily");
        BehaviorPreference::instance()->setProperty("fontFamily", m_widget->property("fontFamily"));
        qCDebug(lcAppearancePage) << "fontStyle" << m_widget->property("fontStyle");
        BehaviorPreference::instance()->setProperty("fontStyle", m_widget->property("fontStyle"));
        qCDebug(lcAppearancePage) << "uiBehavior" << m_widget->property("uiBehavior");
        BehaviorPreference::instance()->setProperty("uiBehavior", m_widget->property("uiBehavior"));
        qCDebug(lcAppearancePage) << "graphicsBehavior" << m_widget->property("graphicsBehavior");
        BehaviorPreference::instance()->setProperty("graphicsBehavior", m_widget->property("graphicsBehavior"));
        qCDebug(lcAppearancePage) << "animationEnabled" << m_widget->property("animationEnabled");
        BehaviorPreference::instance()->setProperty("animationEnabled", m_widget->property("animationEnabled"));
        qCDebug(lcAppearancePage) << m_widget->property("animationSpeedRatio");
        BehaviorPreference::instance()->setProperty("animationSpeedRatio", m_widget->property("animationSpeedRatio"));
        BehaviorPreference::instance()->save();
        return ISettingPage::accept();
    }
    void AppearancePage::endSetting() {
        qCInfo(lcAppearancePage) << "Ending setting";
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
