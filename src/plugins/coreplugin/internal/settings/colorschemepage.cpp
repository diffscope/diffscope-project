#include "colorschemepage.h"

#include <QApplication>
#include <QQmlComponent>
#include <CoreApi/plugindatabase.h>

namespace Core::Internal {
    ColorSchemePage::ColorSchemePage(QObject *parent)
        : ISettingPage("core.ColorScheme", parent) {
        setTitle(tr("Color Scheme"));
        setDescription(tr("Configure the colors and visual effects of various components"));
    }

    ColorSchemePage::~ColorSchemePage() {
        delete m_widget;
    }
    bool ColorSchemePage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString ColorSchemePage::sortKeyword() const {
        return QStringLiteral("ColorScheme");
    }

    QObject *ColorSchemePage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "ColorSchemePage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({
            {"pageHandle", QVariant::fromValue(this)}
        });
        m_widget->setParent(this);
        return m_widget;
    }
    void ColorSchemePage::beginSetting() {
        widget();
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }
    bool ColorSchemePage::accept() {
        return ISettingPage::accept();
    }

    void ColorSchemePage::endSetting() {
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool ColorSchemePage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }
}