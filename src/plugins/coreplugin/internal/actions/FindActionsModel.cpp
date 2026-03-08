#include "FindActionsModel.h"

#include <algorithm>
#include <ranges>

#include <QKeySequence>
#include <QLoggingCategory>
#include <QPointer>
#include <QSettings>

#include <QtQuickTemplates2/private/qquickaction_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>

#include <QAKCore/actionregistry.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/ActionHelper.h>
#include <coreplugin/internal/FindActionsAddOn.h>
#include <coreplugin/ActionWindowInterfaceBase.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcFindActionsModel, "diffscope.core.findactionsmodel")

    FindActionsModel::FindActionsModel(QAK::QuickActionContext *actionContext, FindActionsAddOn *parent)
        : QAbstractItemModel(parent), m_actionContext(actionContext) {
        m_collator.setNumericMode(true);
        m_collator.setCaseSensitivity(Qt::CaseInsensitive);
    }

    QModelIndex FindActionsModel::index(int row, int column, const QModelIndex &parent) const {
        if (parent.isValid() || column != 0 || row < 0 || row >= m_actionList.size()) {
            return QModelIndex();
        }
        return createIndex(row, column);
    }

    QModelIndex FindActionsModel::parent(const QModelIndex &child) const {
        Q_UNUSED(child)
        return QModelIndex();
    }

    int FindActionsModel::rowCount(const QModelIndex &parent) const {
        if (parent.isValid()) {
            return 0;
        }
        return m_actionList.size();
    }

    int FindActionsModel::columnCount(const QModelIndex &parent) const {
        Q_UNUSED(parent)
        return 1;
    }

    QString getTextWithFallback(const QString &id, bool translated) {
        auto actionInfo = CoreInterface::actionRegistry()->actionInfo(id);
        if (actionInfo.isNull()) {
            return QString();
        }

        QString text = actionInfo.text(translated);
        if (text.isEmpty() && translated) {
            text = actionInfo.text(false);
        }
        return text;
    }

    QString getClassWithFallback(const QString &id, bool translated) {
        auto actionInfo = CoreInterface::actionRegistry()->actionInfo(id);
        if (actionInfo.isNull()) {
            return QString();
        }

        QString clazz = actionInfo.actionClass(translated);
        if (clazz.isEmpty() && translated) {
            clazz = actionInfo.actionClass(false);
        }
        return clazz;
    }

    QString getDescriptionWithFallback(const QString &id, bool translated) {
        auto actionInfo = CoreInterface::actionRegistry()->actionInfo(id);
        if (actionInfo.isNull()) {
            return QString();
        }

        QString description = actionInfo.description(translated);
        if (description.isEmpty() && translated) {
            description = actionInfo.description(false);
        }
        return description;
    }

    QStringList getShortcutsAsStringList(const QString &id) {
        auto actionInfo = CoreInterface::actionRegistry()->actionInfo(id);
        if (actionInfo.isNull()) {
            return QStringList();
        }

        const auto shortcuts = actionInfo.shortcuts();
        QStringList result;
        result.reserve(shortcuts.size());

        for (const QKeySequence &shortcut : shortcuts) {
            result.append(shortcut.toString(QKeySequence::NativeText));
        }

        return result;
    }

    enum ActionInternalFlag {
        None,
        Checkable,
        Separator,
        Menu,
    };

    QVariant FindActionsModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() >= m_actionList.size()) {
            return QVariant();
        }

        const auto &[actionId, flag] = m_actionList.at(index.row());

        switch (role) {
            case Qt::DisplayRole:
                return actionId;
            case SVS::SVSCraft::CP_TitleRole: {
                auto text = ActionHelper::removeMnemonic(getTextWithFallback(actionId, true));
                if (flag == Checkable) {
                    text = tr("Toggle \"%1\"").arg(text);
                } else if (flag == Menu) {
                    text = tr("Open Menu \"%1\"...").arg(text);
                }
                auto clazz = getClassWithFallback(actionId, true);
                if (clazz.isEmpty())
                    return text;
                return tr("%1: %2").arg(clazz, text);
            }

            case SVS::SVSCraft::CP_SubtitleRole:
                return QString();

            case SVS::SVSCraft::CP_KeywordRole:
                return actionId;

            case SVS::SVSCraft::CP_DescriptionRole:
                return getDescriptionWithFallback(actionId, true);

            case SVS::SVSCraft::CP_KeySequenceRole:
                return getShortcutsAsStringList(actionId);

            case SVS::SVSCraft::CP_TagRole:
                return m_priorityActions.contains(actionId) ? tr("recently used") : "";

            case SVS::SVSCraft::CP_IsSeparatorRole:
                return flag == Separator;

            default:
                return QVariant();
        }
    }

    static bool includeActionPredicate(const QString &id) {
        static const QString namespaceUri = "http://schemas.diffscope.org/diffscope/actions/diffscope";
        auto info = CoreInterface::actionRegistry()->actionInfo(id);
        if (info.attributes().contains(QAK::ActionAttributeKey("excludeFromCommands", namespaceUri))) {
            return false;
        }
        auto componentType = info.attributes().value(QAK::ActionAttributeKey("componentType", namespaceUri));
        if (!componentType.isEmpty() && componentType != "action" && componentType != "menu") {
            return false;
        }
        return true;
    }

    void FindActionsModel::setActions(const QStringList &actions) {
        auto v = actions | std::views::filter(includeActionPredicate);
        m_actions = QStringList(v.begin(), v.end());
    }

    void FindActionsModel::setPriorityActions(const QStringList &priorityActions) {
        m_priorityActions = priorityActions;
    }

    void FindActionsModel::refresh() {
        updateActionList();
    }

    void FindActionsModel::trigger(int index, ActionWindowInterfaceBase *windowInterface) {
        const auto &[actionId, flag] = m_actionList.at(index);
        if (flag == Menu) {
            windowInterface->execQuickPick(static_cast<QQuickMenu *>(getActionObject(actionId)));
        } else {
            windowInterface->triggerAction(actionId, windowInterface->window()->property("contentItem").value<QObject *>());
        }
    }

    void FindActionsModel::updateActionList() {
        beginResetModel();

        m_actionList.clear();

        // Add remaining actions, excluding those already in priority list
        QStringList remainingActions;
        for (const QString &actionId : m_actions) {
            if (!m_priorityActions.contains(actionId)) {
                remainingActions.append(actionId);
            }
        }

        // Sort remaining actions using local collator
        std::sort(remainingActions.begin(), remainingActions.end(), [this](const QString &a, const QString &b) { return m_collator.compare(a, b) < 0; });

        auto getActionFlagPair = [this](const QString &id) -> QPair<QString, int> {
            // TODO avoid creating action object on each time updating action list
            auto actionObject = getActionObject(id);
            if (auto action = qobject_cast<QQuickAction *>(actionObject)) {
                if (!action->isEnabled()) {
                    return {};
                }
                return {id, action->isCheckable() ? Checkable : None};
            }
            if (auto menu = qobject_cast<QQuickMenu *>(actionObject)) {
                if (!menu->isEnabled()) {
                    return {};
                }
                return {id, Menu};
            }
            qCWarning(lcFindActionsModel) << "Action" << id << "is neither an action or a menu. Please declare `componentType` in the action manifest.";
            return {};
        };

        auto pipeline = std::views::transform(getActionFlagPair) | std::views::filter([](const auto &p) { return !p.first.isEmpty(); });
        for (const auto &item : m_priorityActions | pipeline) {
            m_actionList.append(item);
        }
        if (!m_priorityActions.isEmpty()) {
            m_actionList.append({{}, Separator});
        }
        for (const auto &item : remainingActions | pipeline) {
            m_actionList.append(item);
        }

        endResetModel();
    }
    QObject *FindActionsModel::getActionObject(const QString &id) {
        if (m_actionObjects.value(id)) {
            return m_actionObjects.value(id);
        } else {
            auto obj = ActionHelper::createActionObject(m_actionContext, id, false);
            if (!obj)
                return nullptr;
            auto window = static_cast<FindActionsAddOn *>(QObject::parent())->windowHandle()->window();
            obj->setParent(window);
            m_actionObjects.insert(id, obj);
            return m_actionObjects.value(id);
        }
    }

}
