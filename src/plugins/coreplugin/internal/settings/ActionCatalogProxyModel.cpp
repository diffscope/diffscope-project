// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ActionCatalogProxyModel.h"

#include <QColor>
#include <QStringList>
#include <QUrl>

#include <QAKCore/actioncatalogmodel.h>
#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>

namespace Core::Internal {

    enum CatalogRole {
        TitleRole = Qt::UserRole + 1,
        TypeTextRole,
        CommandRole,
        EntryTypeRole,
        ShortcutRole,
        CustomizedRole,
        IconSourceRole,
        IconColorRole,
    };

    ActionCatalogProxyModel::ActionCatalogProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
        auto catalog = new QAK::ActionCatalogModel(this);
        catalog->setCatalog(CoreInterface::actionRegistry()->catalog());
        setSourceModel(catalog);
        setRecursiveFilteringEnabled(true);
    }

    QKeySequence ActionCatalogProxyModel::shortcut(const QString &id) const {
        if (const auto it = m_shortcutOverrides.constFind(id); it != m_shortcutOverrides.cend()) {
            return it.value();
        }
        const auto info = CoreInterface::actionRegistry()->actionInfo(id);
        return info ? info->shortcuts().value(0) : QKeySequence();
    }

    QVariant ActionCatalogProxyModel::data(const QModelIndex &index, int role) const {
        const auto source = mapToSource(index);
        const auto id = source.data().toString();
        const auto info = CoreInterface::actionRegistry()->actionInfo(id);
        if (!info) {
            switch (role) {
                case TitleRole:
                    return id;
                case TypeTextRole:
                    return QString();
                case CommandRole:
                case CustomizedRole:
                    return false;
                case EntryTypeRole:
                    return -1;
                case ShortcutRole:
                    return QString();
                case IconSourceRole:
                    return QUrl();
                case IconColorRole:
                    return QColor();
                default:
                    break;
            }
            return QSortFilterProxyModel::data(index, role);
        }
        const int type = info->type() == QAK::ActionItemInfo::Menu ? QAK::ActionLayoutEntry::Menu :
                         info->type() == QAK::ActionItemInfo::Group ? QAK::ActionLayoutEntry::Group :
                         info->type() == QAK::ActionItemInfo::Action ? QAK::ActionLayoutEntry::Action : -1;
        switch (role) {
            case TitleRole: {
                const auto text = info->text().withoutMnemonic();
                return text.isEmpty() ? id : text;
            }
            case TypeTextRole:
                return typeText(id, type);
            case CommandRole:
                return info->isCommand();
            case EntryTypeRole:
                return type;
            case ShortcutRole:
                return shortcut(id).toString(QKeySequence::NativeText);
            case CustomizedRole:
                return m_shortcutOverrides.contains(id);
            case IconSourceRole:
            case IconColorRole: {
                const auto icon = CoreInterface::actionRegistry()->actionIcon({}, id, info->icon()).value_or(QAK::ActionIcon());
                return role == IconSourceRole ? QVariant(icon.url()) : QVariant(QColor::fromString(icon.currentColor()));
            }
            default:
                return QSortFilterProxyModel::data(index, role);
        }
    }

    QHash<int, QByteArray> ActionCatalogProxyModel::roleNames() const {
        auto roles = QSortFilterProxyModel::roleNames();
        roles[TitleRole] = "title";
        roles[TypeTextRole] = "typeText";
        roles[CommandRole] = "command";
        roles[EntryTypeRole] = "entryType";
        roles[ShortcutRole] = "shortcut";
        roles[CustomizedRole] = "customized";
        roles[IconSourceRole] = "iconSource";
        roles[IconColorRole] = "iconColor";
        return roles;
    }

    QVariantMap ActionCatalogProxyModel::itemInfo(const QModelIndex &index) const {
        return {
            {"id", data(index, Qt::DisplayRole)},
            {"entryType", data(index, EntryTypeRole)},
            {"command", data(index, CommandRole)},
        };
    }

    QModelIndex ActionCatalogProxyModel::indexForId(const QString &id) const {
        const auto catalog = static_cast<QAK::ActionCatalogModel *>(sourceModel())->catalog();
        QStringList path;
        for (auto current = id; !current.isEmpty();) {
            path.prepend(current);
            const auto parent = catalog.parent(current);
            if (!parent) {
                return {};
            }
            current = *parent;
        }
        QString parentId;
        QModelIndex parentIndex;
        for (const auto &child : path) {
            const int row = int(catalog.children(parentId).indexOf(child));
            if (row < 0) {
                return {};
            }
            parentIndex = sourceModel()->index(row, 0, parentIndex);
            parentId = child;
        }
        return mapFromSource(parentIndex);
    }

    QString ActionCatalogProxyModel::searchText() const {
        return m_searchText;
    }

    void ActionCatalogProxyModel::setSearchText(const QString &text) {
        if (m_searchText == text) {
            return;
        }
        m_searchText = text;
        invalidateFilter();
        Q_EMIT searchTextChanged();
    }

    void ActionCatalogProxyModel::setCommandsOnly(bool commandsOnly) {
        m_commandsOnly = commandsOnly;
        invalidateFilter();
    }

    void ActionCatalogProxyModel::setShortcutOverrides(const KeymapOverrides &overrides) {
        beginResetModel();
        m_shortcutOverrides = overrides;
        endResetModel();
        invalidateFilter();
    }

    void ActionCatalogProxyModel::setShortcutFilter(const QKeySequence &sequence) {
        m_shortcutFilter = sequence;
        invalidateFilter();
    }

    void ActionCatalogProxyModel::refresh() {
        static_cast<QAK::ActionCatalogModel *>(sourceModel())->setCatalog(CoreInterface::actionRegistry()->catalog());
    }

    bool ActionCatalogProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const {
        const auto index = sourceModel()->index(row, 0, parent);
        const auto id = index.data().toString();
        const auto info = CoreInterface::actionRegistry()->actionInfo(id);
        if (!info) {
            return false;
        }
        if (m_commandsOnly && !info->isCommand()) {
            return false;
        }
        const auto key = shortcut(id);
        if (!m_shortcutFilter.isEmpty() && key != m_shortcutFilter) {
            return false;
        }
        QStringList text{id, info->text().withoutMnemonic(), info->shortText().withoutMnemonic(), info->description().toString()};
        text.append(key.toString(QKeySequence::NativeText));
        text.append(key.toString(QKeySequence::PortableText));
        if (text.join(u'\n').contains(m_searchText.trimmed(), Qt::CaseInsensitive)) {
            return true;
        }
        for (auto ancestor = parent; ancestor.isValid(); ancestor = ancestor.parent()) {
            const auto parentInfo = CoreInterface::actionRegistry()->actionInfo(ancestor.data().toString());
            if (parentInfo && parentInfo->text().withoutMnemonic().contains(m_searchText.trimmed(), Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    QString ActionCatalogProxyModel::typeText(const QString &id, int type) {
        switch (type) {
            case QAK::ActionLayoutEntry::Menu:
                return tr("Menu");
            case QAK::ActionLayoutEntry::Group:
                return tr("Action group");
            case QAK::ActionLayoutEntry::Separator:
                return tr("Separator");
            case QAK::ActionLayoutEntry::Stretch:
                return tr("Stretch");
            case QAK::ActionLayoutEntry::Action: {
                const auto info = CoreInterface::actionRegistry()->actionInfo(id);
                const auto component = info ? info->attributes().value(QAK::ActionAttributeKey("componentType", "http://schemas.diffscope.org/diffscope/actions/diffscope")) : QString();
                if (component.isEmpty() || component == "action") {
                    return tr("Action");
                }
                if (component == "menu") {
                    return tr("Menu");
                }
                if (component == "widget") {
                    return tr("Widget");
                }
                return component == "custom" ? tr("Custom component") : component;
            }
            default:
                return tr("Category");
        }
    }
}

#include "moc_ActionCatalogProxyModel.cpp"
