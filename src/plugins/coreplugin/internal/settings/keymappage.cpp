#include "keymappage.h"

#include <QApplication>
#include <QQmlComponent>
#include <CoreApi/plugindatabase.h>

namespace Core::Internal {
    KeyMapPage::KeyMapPage(QObject *parent)
        : ISettingPage("core.Keymap", parent) {
        setTitle(tr("Keymap"));
        setDescription(tr("Configure shortcuts of actions"));
    }

    KeyMapPage::~KeyMapPage() {
        delete m_widget;
    }
    
    bool KeyMapPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString KeyMapPage::sortKeyword() const {
        return QStringLiteral("Keymap");
    }

    QObject *KeyMapPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "KeymapPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({
            {"pageHandle", QVariant::fromValue(this)}
        });
        m_widget->setParent(this);
        return m_widget;
    }
    
    void KeyMapPage::beginSetting() {
        widget();
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }
    
    bool KeyMapPage::accept() {
        return ISettingPage::accept();
    }

    void KeyMapPage::endSetting() {
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool KeyMapPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", Q_RETURN_ARG(bool, ret), word);
        return ret;
    }
}