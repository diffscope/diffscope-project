#ifndef UISHELL_SETTINGCATALOGMODEL_P_H
#define UISHELL_SETTINGCATALOGMODEL_P_H

#include <QAbstractItemModel>
#include <qqmlintegration.h>

namespace Core {
    class SettingCatalog;
    class ISettingPage;
}

namespace UIShell {

    class SettingCatalogModel : public QAbstractItemModel {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(Core::SettingCatalog *settingCatalog READ settingCatalog WRITE setSettingCatalog NOTIFY settingCatalogChanged)
    public:
        explicit SettingCatalogModel(QObject *parent = nullptr);
        ~SettingCatalogModel() override;

        // QAbstractItemModel interface
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &index) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        // Set the setting catalog to use as data source
        void setSettingCatalog(Core::SettingCatalog *catalog);
        Core::SettingCatalog *settingCatalog() const;

    signals:
        void settingCatalogChanged();

    private slots:
        void onPageAdded(Core::ISettingPage *page);
        void onPageRemoved(Core::ISettingPage *page);

    private:
        Core::ISettingPage *pageForIndex(const QModelIndex &index) const;
        QModelIndex indexForPage(Core::ISettingPage *page) const;
        int rowOfPage(Core::ISettingPage *page) const;

        Core::SettingCatalog *m_catalog;
    };

}

#endif //UISHELL_SETTINGCATALOGMODEL_P_H
