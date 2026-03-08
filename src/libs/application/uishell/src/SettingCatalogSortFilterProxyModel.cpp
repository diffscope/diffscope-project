#include "SettingCatalogSortFilterProxyModel_p.h"

#include <CoreApi/isettingpage.h>

namespace UIShell {

    SettingCatalogSortFilterProxyModel::SettingCatalogSortFilterProxyModel(QObject *parent)
        : QSortFilterProxyModel(parent) {
        // Enable sorting
        setSortRole(Qt::DisplayRole);
        setDynamicSortFilter(true);
        setAutoAcceptChildRows(true);
        setRecursiveFilteringEnabled(true);
    }

    QString SettingCatalogSortFilterProxyModel::filterKeyword() const {
        return m_filterKeyword;
    }

    void SettingCatalogSortFilterProxyModel::setFilterKeyword(const QString &keyword) {
        if (m_filterKeyword == keyword) {
            return;
        }

        m_filterKeyword = keyword;
        emit filterKeywordChanged();

        // Trigger filter update
        invalidateFilter();
    }

    bool SettingCatalogSortFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
        return source_left.row() < source_right.row();
    }

    bool SettingCatalogSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
        // If no filter keyword is set, accept all rows
        if (m_filterKeyword.isEmpty()) {
            return true;
        }

        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!index.isValid()) {
            return false;
        }

        Core::ISettingPage *page = pageForIndex(index);
        if (!page) {
            return false;
        }

        // Use ISettingPage::matches() to check if the page matches the filter keyword
        return page->matches(m_filterKeyword);
    }

    Core::ISettingPage *SettingCatalogSortFilterProxyModel::pageForIndex(const QModelIndex &index) const {
        if (!index.isValid()) {
            return nullptr;
        }

        // Get the page from the model data
        QVariant data = sourceModel()->data(index, Qt::DisplayRole);
        if (!data.isValid()) {
            return nullptr;
        }

        // The SettingCatalogModel returns ISettingPage* as QVariant
        return data.value<Core::ISettingPage *>();
    }

    QModelIndex SettingCatalogSortFilterProxyModel::findFirstMatch() const {
        if (!sourceModel()) {
            return {};
        }

        // Start depth-first search from the root
        return findFirstMatchRecursive(QModelIndex());
    }

    QModelIndex SettingCatalogSortFilterProxyModel::findFirstMatchRecursive(const QModelIndex &parent) const {
        if (!sourceModel()) {
            return {};
        }

        int rowCount = sourceModel()->rowCount(parent);

        for (int row = 0; row < rowCount; ++row) {
            QModelIndex sourceIndex = sourceModel()->index(row, 0, parent);
            if (!sourceIndex.isValid()) {
                continue;
            }

            // Check if current item matches
            Core::ISettingPage *page = pageForIndex(sourceIndex);
            if (filterKeyword().isEmpty() || (page && page->matches(m_filterKeyword))) {
                // Map from source model index to proxy model index
                return mapFromSource(sourceIndex);
            }

            // Recursively search children (depth-first)
            QModelIndex childMatch = findFirstMatchRecursive(sourceIndex);
            if (childMatch.isValid()) {
                return childMatch;
            }
        }

        return {};
    }

}
