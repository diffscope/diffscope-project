#include "menupage.h"

#include <QApplication>
#include <QQmlComponent>

#include <QAKCore/actionlayoutsmodel.h>
#include <QAKCore/actionregistry.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>

namespace Core::Internal {
    MenuPage::MenuPage(QObject *parent)
        : ISettingPage("core.Menu", parent) {
        setTitle(tr("Menus and Toolbars"));
        setDescription(tr("Configure the layout of menus and toolbars"));
    }

    MenuPage::~MenuPage() {
        delete m_widget;
    }
    
    bool MenuPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString MenuPage::sortKeyword() const {
        return QStringLiteral("Menus");
    }

    QObject *MenuPage::widget() {
        if (m_widget)
            return m_widget;
        m_actionLayoutsModel = new QAK::ActionLayoutsModel(this);
        QVector<QAK::ActionLayoutEntry> topLevelNodes;
        for (const auto &id: ICore::actionRegistry()->actionIds()) {
            auto info = ICore::actionRegistry()->actionInfo(id);
            if (info.topLevel()) {
                topLevelNodes.append(QAK::ActionLayoutEntry(id, QAK::ActionLayoutEntry::Menu));
            }
        }
        m_actionLayoutsModel->setTopLevelNodes(topLevelNodes);
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "MenuPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({
            {"pageHandle", QVariant::fromValue(this)},
            {"model", QVariant::fromValue(m_actionLayoutsModel)}
        });
        m_widget->setParent(this);
        return m_widget;
    }
    
    void MenuPage::beginSetting() {
        widget();
        m_actionLayoutsModel->setActionLayouts(ICore::actionRegistry()->layouts());
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }
    
    bool MenuPage::accept() {
        return ISettingPage::accept();
    }

    void MenuPage::endSetting() {
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool MenuPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", Q_RETURN_ARG(bool, ret), word);
        return ret;
    }
}