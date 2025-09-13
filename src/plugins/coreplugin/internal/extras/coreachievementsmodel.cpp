#include "coreachievementsmodel.h"

#include <QUrl>

#include <uishell/USDef.h>

namespace Core::Internal {

    static CoreAchievementsModel *m_instance = nullptr;
    CoreAchievementsModel::CoreAchievementsModel(QObject *parent) : QStandardItemModel(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        auto header = new QStandardItem(tr("Core"));
        setHorizontalHeaderItem(0, header);
        {
            auto item = new QStandardItem;
            item->setData("core.diffscope", UIShell::USDef::AR_IdRole);
            item->setData(tr("Welcome to DiffScope"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Enjoy creating music!"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("image://appicon/app"), UIShell::USDef::AR_IconRole);
            item->setData(QColor(Qt::transparent), UIShell::USDef::AR_IconColorRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.newProject", UIShell::USDef::AR_IdRole);
            item->setData(tr("Will It Be This Year's Top Hit?"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Create a new project"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("image://action/core.file.new"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.findActions", UIShell::USDef::AR_IdRole);
            item->setData(tr("What Can I Do?"), UIShell::USDef::AR_NameRole);
            item->setData(tr("\"Find Actions\" will answer you"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("image://action/core.findActions"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.help", UIShell::USDef::AR_IdRole);
            item->setData(tr("What Else Can I Do?"), UIShell::USDef::AR_NameRole);
            item->setData(tr("\"Help\" will answer you"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("image://action/core.documentations"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.contextHelpTip", UIShell::USDef::AR_IdRole);
            item->setData(tr("Hold [W] to Ponder"), UIShell::USDef::AR_NameRole);
            item->setData(tr("A much better way to show context help than \"What's This?\""), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("image://action/core.panel.tips"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.disableCustomTitleBar", UIShell::USDef::AR_IdRole);
            item->setData(tr("Native Look and Feel"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Disable the custom title bar"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/Window16Filled.svg"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.disableAnimation", UIShell::USDef::AR_IdRole);
            item->setData(tr("Make Full Use of Graphics Performance"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Disable animation"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/CircleOff16Filled.svg"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.commandLineSettings", UIShell::USDef::AR_IdRole);
            item->setData(tr("Open Settings with Command Line"), UIShell::USDef::AR_NameRole);
            item->setData(tr("An alternative solution if you accidentally removed it from the main menu"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("image://action/core.settings"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.plugins", UIShell::USDef::AR_IdRole);
            item->setData(tr("Extensibility"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Have a look at the plugin list"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("image://action/core.plugins"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.zjzj", UIShell::USDef::AR_IdRole);
            item->setData(tr("Ultimate Simplicity"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Disable all plugins, except Core (and Achievement, of course)"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/Flash16Filled.svg"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.keepPatient", UIShell::USDef::AR_IdRole);
            item->setData(tr("Keep Patient"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Why is it taking so long to load?"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/AnimalTurtle16Filled.svg"), UIShell::USDef::AR_IconRole);
            item->setData(true, UIShell::USDef::AR_HiddenRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.movePanel", UIShell::USDef::AR_IdRole);
            item->setData(tr("Better Organized"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Drag a panel somewhere else"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/Drag24Filled.svg"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.undockPanel", UIShell::USDef::AR_IdRole);
            item->setData(tr("Way Better Organized"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Eject a panel outside the project window"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/PanelSeparateWindow20Filled.svg"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.removePanel", UIShell::USDef::AR_IdRole);
            item->setData(tr("Better than Way Better Organized"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Remove a panel from the workspace"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/Delete16Filled.svg"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.newPanel", UIShell::USDef::AR_IdRole);
            item->setData(tr("Way Better than Way Better Organized"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Bring something new to the workspace"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/AddCircle16Filled.svg"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.quickJump", UIShell::USDef::AR_IdRole);
            item->setData(tr("Quick Jump"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Quickly navigate to a specific position"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/TargetArrow16Filled.svg"), UIShell::USDef::AR_IconRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.9bang15", UIShell::USDef::AR_IdRole);
            item->setData(QStringLiteral("9bang15\u4fbf\u58eb"), UIShell::USDef::AR_NameRole);
            item->setData(QStringLiteral("\u886c\u886b\u7684\u4ef7\u683c\u662f\u4e5d\u78c5\u5341\u4e94\u4fbf\u58eb"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/Money16Filled.svg"), UIShell::USDef::AR_IconRole);
            item->setData(true, UIShell::USDef::AR_HiddenRole);
            appendRow(item);
        }
        {
            auto item = new QStandardItem;
            item->setData("core.ultimateQuestion", UIShell::USDef::AR_IdRole);
            item->setData(tr("42"), UIShell::USDef::AR_NameRole);
            item->setData(tr("Answer to the ultimate question"), UIShell::USDef::AR_DescriptionRole);
            item->setData(QUrl("qrc:/diffscope/coreplugin/icons/4220Filled.svg"), UIShell::USDef::AR_IconRole);
            item->setData(true, UIShell::USDef::AR_HiddenRole);
            appendRow(item);
        }
    }
    CoreAchievementsModel::~CoreAchievementsModel() {
        m_instance = nullptr;
    }
    CoreAchievementsModel *CoreAchievementsModel::instance() {
        return m_instance;
    }
}