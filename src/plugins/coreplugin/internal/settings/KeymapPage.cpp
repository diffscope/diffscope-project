#include "KeymapPage.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcKeyMapPage, "diffscope.core.keymappage")

    KeyMapPage::KeyMapPage(QObject *parent)
        : ISettingPage("org.diffscope.core.Keymap", parent) {
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
        qCDebug(lcKeyMapPage) << "Creating widget";
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "KeymapPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}
        });
        m_widget->setParent(this);
        return m_widget;
    }

    void KeyMapPage::beginSetting() {
        qCInfo(lcKeyMapPage) << "Beginning setting";
        widget();
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool KeyMapPage::accept() {
        qCInfo(lcKeyMapPage) << "Accepting";
        return ISettingPage::accept();
    }

    void KeyMapPage::endSetting() {
        qCInfo(lcKeyMapPage) << "Ending setting";
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
