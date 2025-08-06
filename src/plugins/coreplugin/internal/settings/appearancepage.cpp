#include "appearancepage.h"

#include <QApplication>
#include <QQmlComponent>

#include <coreplugin/icore.h>

namespace Core::Internal {
    AppearancePage::AppearancePage(QObject *parent) : ISettingPage("core.Appearance", parent) {
        setTitle(tr("Appearance"));
        setDescription(tr("Configure how %1 looks like").arg(QApplication::applicationName()));
    }
    AppearancePage::~AppearancePage() = default;
    QString AppearancePage::sortKeyword() const {
        return QStringLiteral("Appearance");
    }
    QObject *AppearancePage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "AppearancePage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.create();
        m_widget->setParent(this);
        return m_widget;
    }
    void AppearancePage::beginSetting() {
        ISettingPage::beginSetting();
    }
    bool AppearancePage::accept() {
        return ISettingPage::accept();
    }
    void AppearancePage::endSetting() {
        ISettingPage::endSetting();
    }
}