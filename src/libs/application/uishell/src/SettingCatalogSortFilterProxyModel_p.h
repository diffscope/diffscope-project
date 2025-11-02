#ifndef UISHELL_SETTINGCATALOGSORTFILTERPROXYMODEL_P_H
#define UISHELL_SETTINGCATALOGSORTFILTERPROXYMODEL_P_H

#include <qqmlintegration.h>

#include <QSortFilterProxyModel>

namespace Core {
    class ISettingPage;
}

namespace UIShell {

    class SettingCatalogSortFilterProxyModel : public QSortFilterProxyModel {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QString filterKeyword READ filterKeyword WRITE setFilterKeyword NOTIFY filterKeywordChanged)
    public:
        explicit SettingCatalogSortFilterProxyModel(QObject *parent = nullptr);

        QString filterKeyword() const;
        void setFilterKeyword(const QString &keyword);

        Q_INVOKABLE static inline QPersistentModelIndex toPersistentModelIndex(const QModelIndex &index) {
            return index;
        }

        Q_INVOKABLE QModelIndex findFirstMatch() const;

    signals:
        void filterKeywordChanged();

    protected:
        bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        Core::ISettingPage *pageForIndex(const QModelIndex &index) const;
        QModelIndex findFirstMatchRecursive(const QModelIndex &parent) const;

        QString m_filterKeyword;
    };

}

#endif //UISHELL_SETTINGCATALOGSORTFILTERPROXYMODEL_P_H
