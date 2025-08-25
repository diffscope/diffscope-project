#include "findactionsmodel.h"

#include <algorithm>
#include <ranges>

#include <QKeySequence>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/icore.h>
#include <coreplugin/internal/actionhelper.h>

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

    static QString removeMnemonic(const QString &s) {
        QString text(s.size(), QChar::Null);
        int idx = 0;
        int pos = 0;
        int len = s.size();
        while (len) {
            if (s.at(pos) == QLatin1Char('&') && (len == 1 || s.at(pos + 1) != QLatin1Char('&'))) {
                ++pos;
                --len;
                if (len == 0)
                    break;
            } else if (s.at(pos) == QLatin1Char('(') && len >= 4 &&
                       s.at(pos + 1) == QLatin1Char('&') &&
                       s.at(pos + 2) != QLatin1Char('&') &&
                       s.at(pos + 3) == QLatin1Char(')')) {
                // a mnemonic with format "\s*(&X)"
                int n = 0;
                while (idx > n && text.at(idx - n - 1).isSpace())
                    ++n;
                idx -= n;
                pos += 4;
                len -= 4;
                continue;
                       }
            text[idx] = s.at(pos);
            ++pos;
            ++idx;
            --len;
        }
        text.truncate(idx);
        return text;
    }

    enum ActionInternalFlag {
        None,
        Checkable,
        Separator,
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
                auto text = removeMnemonic(getTextWithFallback(actionId, true));
                if (flag == Checkable) {
                    text = tr("Toggle \"%1\"").arg(text);
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

    void FindActionsModel::refresh(QAK::QuickActionContext *actionContext) {
        updateActionList(actionContext);
    }

    void FindActionsModel::updateActionList(QAK::QuickActionContext *actionContext) {
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
        std::sort(remainingActions.begin(), remainingActions.end(), 
                  [this](const QString &a, const QString &b) {
                      return m_collator.compare(a, b) < 0;
                  });


        auto pipeline =
            std::views::transform([=](const QString &id) -> QPair<QString, int> {
                std::unique_ptr<QObject> actionObject(
                    ActionHelper::createActionObject(actionContext, id, true));
                if (!actionObject || !actionObject->property("enabled").toBool()) {
                    return {};
                }
                return {id, actionObject->property("checkable").toBool() ? Checkable : None};
            }) |
            std::views::filter([](const auto &p) { return !p.first.isEmpty(); });
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

}
