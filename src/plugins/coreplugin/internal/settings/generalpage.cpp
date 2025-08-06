#include "generalpage.h"

#include <QApplication>
#include <QQmlComponent>

#include <coreplugin/icore.h>

namespace Core::Internal {
    GeneralPage::GeneralPage(QObject *parent) : ISettingPage("core.General", parent) {
        setTitle(tr("General"));
        setDescription(tr("Configure general behaviors of %1").arg(QApplication::applicationName()));

    }
    GeneralPage::~GeneralPage() = default;
    bool GeneralPage::matches(const QString &word) const {
        return ISettingPage::matches(word);
    }
    QString GeneralPage::sortKeyword() const {
        return QStringLiteral("General");
    }
    QObject *GeneralPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "GeneralPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.create();
        m_widget->setParent(this);
        return m_widget;
    }
    void GeneralPage::beginSetting() {
        ISettingPage::beginSetting();
    }
    bool GeneralPage::accept() {
        return ISettingPage::accept();
    }
    void GeneralPage::endSetting() {
        ISettingPage::endSetting();
    }

}
