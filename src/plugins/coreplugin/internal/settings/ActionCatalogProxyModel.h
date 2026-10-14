// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_ACTIONCATALOGPROXYMODEL_H
#define DIFFSCOPE_COREPLUGIN_ACTIONCATALOGPROXYMODEL_H

#include <QSortFilterProxyModel>

#include <coreplugin/internal/ActionConfiguration.h>

namespace Core::Internal {

    class ActionCatalogProxyModel : public QSortFilterProxyModel {
        Q_OBJECT
        Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    public:
        explicit ActionCatalogProxyModel(QObject *parent = nullptr);
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QHash<int, QByteArray> roleNames() const override;

        QString searchText() const;
        void setSearchText(const QString &text);
        void setCommandsOnly(bool commandsOnly);
        void setShortcutOverrides(const KeymapOverrides &overrides);
        void setShortcutFilter(const QKeySequence &sequence);
        void refresh();
        Q_INVOKABLE QVariantMap itemInfo(const QModelIndex &index) const;
        Q_INVOKABLE QModelIndex indexForId(const QString &id) const;

        static QString typeText(const QString &id, int type);

    Q_SIGNALS:
        void searchTextChanged();

    protected:
        bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

    private:
        QString m_searchText;
        bool m_commandsOnly = false;
        QKeySequence m_shortcutFilter;
        KeymapOverrides m_shortcutOverrides;
        QKeySequence shortcut(const QString &id) const;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_ACTIONCATALOGPROXYMODEL_H
