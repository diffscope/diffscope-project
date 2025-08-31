#include "timeindicatorpage.h"

#include <QApplication>
#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/internal/behaviorpreference.h>

namespace Core::Internal {
    TimeIndicatorPage::TimeIndicatorPage(QObject *parent) : ISettingPage("core.TimeIndicator", parent) {
        setTitle(tr("Time Indicator"));
        setDescription(tr("Configure time indicator display and interaction behaviors"));
    }

    TimeIndicatorPage::~TimeIndicatorPage() {
        delete m_widget;
    }

    bool TimeIndicatorPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString TimeIndicatorPage::sortKeyword() const {
        return QStringLiteral("TimeIndicator");
    }

    QObject *TimeIndicatorPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "TimeIndicatorPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void TimeIndicatorPage::beginSetting() {
        widget();
        m_widget->setProperty("timeIndicatorBackgroundVisible", BehaviorPreference::instance()->property("timeIndicatorBackgroundVisible"));
        m_widget->setProperty("timeIndicatorClickAction", BehaviorPreference::instance()->property("timeIndicatorClickAction"));
        m_widget->setProperty("timeIndicatorDoubleClickAction", BehaviorPreference::instance()->property("timeIndicatorDoubleClickAction"));
        m_widget->setProperty("timeIndicatorPressAndHoldAction", BehaviorPreference::instance()->property("timeIndicatorPressAndHoldAction"));
        m_widget->setProperty("timeIndicatorTextFineTuneEnabled", BehaviorPreference::instance()->property("timeIndicatorTextFineTuneEnabled"));
        m_widget->setProperty("timeIndicatorShowSliderOnHover", BehaviorPreference::instance()->property("timeIndicatorShowSliderOnHover"));
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool TimeIndicatorPage::accept() {
        BehaviorPreference::instance()->setProperty("timeIndicatorBackgroundVisible", m_widget->property("timeIndicatorBackgroundVisible"));
        BehaviorPreference::instance()->setProperty("timeIndicatorClickAction", m_widget->property("timeIndicatorClickAction"));
        BehaviorPreference::instance()->setProperty("timeIndicatorDoubleClickAction", m_widget->property("timeIndicatorDoubleClickAction"));
        BehaviorPreference::instance()->setProperty("timeIndicatorPressAndHoldAction", m_widget->property("timeIndicatorPressAndHoldAction"));
        BehaviorPreference::instance()->setProperty("timeIndicatorTextFineTuneEnabled", m_widget->property("timeIndicatorTextFineTuneEnabled"));
        BehaviorPreference::instance()->setProperty("timeIndicatorShowSliderOnHover", m_widget->property("timeIndicatorShowSliderOnHover"));
        BehaviorPreference::instance()->save();
        return ISettingPage::accept();
    }

    void TimeIndicatorPage::endSetting() {
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool TimeIndicatorPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
