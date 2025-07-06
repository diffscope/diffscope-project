#include "SettingCatalogModel_p.h"

#include <CoreApi/settingcatalog.h>
#include <CoreApi/isettingpage.h>

namespace UIShell {

    SettingCatalogModel::SettingCatalogModel(QObject *parent)
        : QAbstractItemModel(parent), m_catalog(nullptr) {
    }

    SettingCatalogModel::~SettingCatalogModel() = default;

    QModelIndex SettingCatalogModel::index(int row, int column, const QModelIndex &parent) const {
        if (!hasIndex(row, column, parent)) {
            return {};
        }

        Core::ISettingPage *parentPage = pageForIndex(parent);
        Core::ISettingPage *childPage = nullptr;

        if (parentPage == nullptr) {
            // Top-level pages from catalog
            if (m_catalog) {
                auto pages = m_catalog->pages();
                if (row < pages.size()) {
                    childPage = pages[row];
                }
            }
        } else {
            // Child pages from parent page
            auto pages = parentPage->pages();
            if (row < pages.size()) {
                childPage = pages[row];
            }
        }

        if (childPage) {
            return createIndex(row, column, childPage);
        }

        return {};
    }

    QModelIndex SettingCatalogModel::parent(const QModelIndex &index) const {
        if (!index.isValid()) {
            return {};
        }

        Core::ISettingPage *childPage = pageForIndex(index);
        if (!childPage) {
            return {};
        }

        Core::ISettingPage *parentPage = childPage->parentPage();
        if (!parentPage) {
            return {};
        }

        // Check if parent is a top-level page
        if (m_catalog && m_catalog->pages().contains(parentPage)) {
            int row = m_catalog->pages().indexOf(parentPage);
            return createIndex(row, 0, parentPage);
        }

        // Parent is a child page, find its position in its parent
        Core::ISettingPage *grandParentPage = parentPage->parentPage();
        if (grandParentPage) {
            int row = grandParentPage->pages().indexOf(parentPage);
            return createIndex(row, 0, parentPage);
        } else if (m_catalog && m_catalog->pages().contains(parentPage)) {
            // Parent is a top-level page
            int row = m_catalog->pages().indexOf(parentPage);
            return createIndex(row, 0, parentPage);
        }

        return {};
    }

    int SettingCatalogModel::rowCount(const QModelIndex &parent) const {
        Core::ISettingPage *parentPage = pageForIndex(parent);

        if (parentPage == nullptr) {
            // Top-level pages from catalog
            if (m_catalog) {
                return m_catalog->pages().size();
            }
            return 0;
        }

        // Child pages from parent page
        return parentPage->pages().size();
    }

    int SettingCatalogModel::columnCount(const QModelIndex &parent) const {
        Q_UNUSED(parent)
        return 1;
    }

    QVariant SettingCatalogModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid()) {
            return QVariant();
        }

        Core::ISettingPage *page = pageForIndex(index);
        if (!page) {
            return QVariant();
        }

        switch (role) {
        case Qt::DisplayRole:
            return QVariant::fromValue(page);
        default:
            return QVariant();
        }
    }

    void SettingCatalogModel::setSettingCatalog(Core::SettingCatalog *catalog) {
        if (m_catalog == catalog) {
            return;
        }

        beginResetModel();

        if (m_catalog) {
            disconnect(m_catalog, &Core::SettingCatalog::pageAdded, this, &SettingCatalogModel::onPageAdded);
            disconnect(m_catalog, &Core::SettingCatalog::pageRemoved, this, &SettingCatalogModel::onPageRemoved);
        }

        m_catalog = catalog;

        if (m_catalog) {
            connect(m_catalog, &Core::SettingCatalog::pageAdded, this, &SettingCatalogModel::onPageAdded);
            connect(m_catalog, &Core::SettingCatalog::pageRemoved, this, &SettingCatalogModel::onPageRemoved);
        }

        endResetModel();
        emit settingCatalogChanged();
    }

    Core::SettingCatalog *SettingCatalogModel::settingCatalog() const {
        return m_catalog;
    }

    void SettingCatalogModel::onPageAdded(Core::ISettingPage *page) {
        if (!page || !m_catalog) {
            return;
        }

        // Find the parent of the added page
        Core::ISettingPage *parentPage = page->parentPage();
        
        if (!parentPage) {
            // Top-level page added
            auto pages = m_catalog->pages();
            int row = pages.indexOf(page);
            if (row >= 0) {
                beginInsertRows({}, row, row);
                endInsertRows();
            }
        } else {
            // Child page added
            QModelIndex parentIndex = indexForPage(parentPage);
            if (parentIndex.isValid()) {
                auto pages = parentPage->pages();
                int row = pages.indexOf(page);
                if (row >= 0) {
                    beginInsertRows(parentIndex, row, row);
                    endInsertRows();
                }
            }
        }
    }

    void SettingCatalogModel::onPageRemoved(Core::ISettingPage *page) {
        if (!page || !m_catalog) {
            return;
        }

        // We need to find the page's previous position before it was removed
        // Since the page may already be disconnected from its parent, we need to
        // trigger a full model reset for simplicity
        beginResetModel();
        endResetModel();
    }

    Core::ISettingPage *SettingCatalogModel::pageForIndex(const QModelIndex &index) const {
        if (!index.isValid()) {
            return nullptr;
        }

        return static_cast<Core::ISettingPage *>(index.internalPointer());
    }

    QModelIndex SettingCatalogModel::indexForPage(Core::ISettingPage *page) const {
        if (!page || !m_catalog) {
            return {};
        }

        // Check if it's a top-level page
        auto topLevelPages = m_catalog->pages();
        int topLevelRow = topLevelPages.indexOf(page);
        if (topLevelRow >= 0) {
            return createIndex(topLevelRow, 0, page);
        }

        // Check if it's a child page
        Core::ISettingPage *parentPage = page->parentPage();
        if (parentPage) {
            auto childPages = parentPage->pages();
            int childRow = childPages.indexOf(page);
            if (childRow >= 0) {
                QModelIndex parentIndex = indexForPage(parentPage);
                if (parentIndex.isValid()) {
                    return index(childRow, 0, parentIndex);
                }
            }
        }

        return {};
    }

    int SettingCatalogModel::rowOfPage(Core::ISettingPage *page) const {
        if (!page || !m_catalog) {
            return -1;
        }

        Core::ISettingPage *parentPage = page->parentPage();
        if (!parentPage) {
            // Top-level page
            return m_catalog->pages().indexOf(page);
        }

        // Child page
        return parentPage->pages().indexOf(page);
    }

    static Core::ISettingPage *findPageById(const QList<Core::ISettingPage*> &pages, const QString &id) {
        for (Core::ISettingPage *page : pages) {
            if (page && page->id() == id) {
                return page;
            }
            // Recursively search in child pages
            Core::ISettingPage *foundPage = findPageById(page->pages(), id);
            if (foundPage) {
                return foundPage;
            }
        }
        return nullptr;
    }

    QModelIndex SettingCatalogModel::indexForPageId(const QString &pageId) const {
        if (pageId.isEmpty() || !m_catalog) {
            return {};
        }

        // Search for the page with the specified id
        Core::ISettingPage *targetPage = findPageById(m_catalog->pages(), pageId);
        if (targetPage) {
            return indexForPage(targetPage);
        }

        return {};
    }

}

#include "moc_SettingCatalogModel_p.cpp"