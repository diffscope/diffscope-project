#include "findactionsmodel.h"

#include <algorithm>
#include <ranges>

#include <QKeySequence>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/icore.h>

namespace Core::Internal {

    FindActionsModel::FindActionsModel(QObject *parent)
        : QAbstractItemModel(parent) {
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
        auto actionInfo = ICore::actionRegistry()->actionInfo(id);
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
        auto actionInfo = ICore::actionRegistry()->actionInfo(id);
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
        auto actionInfo = ICore::actionRegistry()->actionInfo(id);
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
        auto actionInfo = ICore::actionRegistry()->actionInfo(id);
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

    QVariant FindActionsModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() >= m_actionList.size()) {
            return QVariant();
        }

        const QString &actionId = m_actionList.at(index.row());

        switch (role) {
            case Qt::DisplayRole:
                return actionId;
            case SVS::SVSCraft::CP_TitleRole: {
                auto text = getTextWithFallback(actionId, true);
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

            default:
                return QVariant();
        }
    }

    void FindActionsModel::setActions(const QStringList &actions) {
        auto v = actions | std::views::filter([](const QString &id) {
            auto info = ICore::actionRegistry()->actionInfo(id);
            return !info.attributes().contains(QAK::ActionAttributeKey("excludeFromCommands", "http://schemas.diffscope.org/diffscope/actions/diffscope"));
        });
        m_actions = QStringList(v.begin(), v.end());
    }

    void FindActionsModel::setPriorityActions(const QStringList &priorityActions) {
        m_priorityActions = priorityActions;
    }

    void FindActionsModel::refresh() {
        updateActionList();
    }

    void FindActionsModel::updateActionList() {
        beginResetModel();

        m_actionList.clear();

        // Add priority actions first, maintaining order
        m_actionList = m_priorityActions;

        // Add remaining actions, excluding those already in priority list
        QStringList remainingActions;
        for (const QString &actionId : m_actions) {
            if (!m_priorityActions.contains(actionId)) {
                remainingActions.append(actionId);
            }
        }

        // Sort remaining actions using local collator
        std::sort(remainingActions.begin(), remainingActions.end(), 
                  [this](const QString &a, const QString &b) {
                      return m_collator.compare(a, b) < 0;
                  });

        // Append sorted remaining actions
        m_actionList.append(remainingActions);

        endResetModel();
    }

}
