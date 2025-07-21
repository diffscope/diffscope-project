#include "LyricsSubtreeProxyModel_p.h"

#include <uishell/USDef.h>

namespace UIShell {

    struct SubtreeProxyItemData {
        QList<int> pathFromRoot; // Path from m_rootIndex to the source item
        int column;

        SubtreeProxyItemData(const QList<int> &path, int col) : pathFromRoot(path), column(col) {}
    };

    LyricsSubtreeProxyModel::LyricsSubtreeProxyModel(QObject *parent)
        : QAbstractProxyModel(parent) {
    }

    LyricsSubtreeProxyModel::~LyricsSubtreeProxyModel() {
        // Cleanup will be handled automatically when the model is destroyed
        // since we're using QModelIndex's built-in cleanup mechanism
    }

    QModelIndex LyricsSubtreeProxyModel::mapFromSource(const QModelIndex &sourceIndex) const {
        if (!sourceModel() || !sourceIndex.isValid()) {
            return QModelIndex();
        }

        // If the source index is not under our root index, it's not mapped
        if (!isValidSourceIndex(sourceIndex)) {
            return QModelIndex();
        }

        return sourceIndexToProxyIndex(sourceIndex);
    }

    QModelIndex LyricsSubtreeProxyModel::mapToSource(const QModelIndex &proxyIndex) const {
        if (!sourceModel() || !proxyIndex.isValid()) {
            return QModelIndex();
        }

        return proxyIndexToSourceIndex(proxyIndex);
    }

    QModelIndex LyricsSubtreeProxyModel::index(int row, int column, const QModelIndex &parent) const {
        if (!sourceModel() || !m_rootIndex.isValid()) {
            return QModelIndex();
        }

        if (row < 0 || column < 0) {
            return QModelIndex();
        }

        // Build path from m_rootIndex to the requested item
        QList<int> pathFromRoot;
        QModelIndex sourceParent = m_rootIndex;
        
        if (parent.isValid()) {
            // Extract path from parent's internal pointer
            SubtreeProxyItemData *parentData = static_cast<SubtreeProxyItemData*>(parent.internalPointer());
            if (parentData) {
                pathFromRoot = parentData->pathFromRoot;
                pathFromRoot.append(parent.row());
                
                // Build sourceParent from the path
                sourceParent = m_rootIndex;
                for (int rowInPath : pathFromRoot) {
                    sourceParent = sourceModel()->index(rowInPath, 0, sourceParent);
                    if (!sourceParent.isValid()) {
                        return QModelIndex();
                    }
                }
            }
        }

        // Verify the source index exists
        QModelIndex sourceIndex = sourceModel()->index(row, column, sourceParent);
        if (!sourceIndex.isValid()) {
            return QModelIndex();
        }

        // Create proxy item data with the current path
        QList<int> currentPath = pathFromRoot;
        currentPath.append(row);
        SubtreeProxyItemData *itemData = new SubtreeProxyItemData(currentPath, column);
        
        return createIndex(row, column, itemData);
    }

    QModelIndex LyricsSubtreeProxyModel::parent(const QModelIndex &child) const {
        if (!sourceModel() || !child.isValid() || !m_rootIndex.isValid()) {
            return QModelIndex();
        }

        SubtreeProxyItemData *itemData = static_cast<SubtreeProxyItemData*>(child.internalPointer());
        if (!itemData || itemData->pathFromRoot.isEmpty()) {
            return QModelIndex(); // This is a top-level item
        }

        // Build parent path by removing the last element
        QList<int> parentPath = itemData->pathFromRoot;
        int parentRow = parentPath.takeLast();
        
        if (parentPath.isEmpty()) {
            // Parent is a direct child of m_rootIndex, so it's a top-level proxy item
            SubtreeProxyItemData *parentData = new SubtreeProxyItemData(QList<int>(), itemData->column);
            return createIndex(parentRow, itemData->column, parentData);
        } else {
            // Parent has its own parent
            SubtreeProxyItemData *parentData = new SubtreeProxyItemData(parentPath, itemData->column);
            return createIndex(parentRow, itemData->column, parentData);
        }
    }

    int LyricsSubtreeProxyModel::rowCount(const QModelIndex &parent) const {
        if (!sourceModel() || !m_rootIndex.isValid()) {
            return 0;
        }

        QModelIndex sourceParent;
        if (parent.isValid()) {
            sourceParent = mapToSource(parent);
        } else {
            sourceParent = m_rootIndex;
        }

        if (!sourceParent.isValid()) {
            return 0;
        }

        return sourceModel()->rowCount(sourceParent);
    }

    int LyricsSubtreeProxyModel::columnCount(const QModelIndex &parent) const {
        if (!sourceModel() || !m_rootIndex.isValid()) {
            return 0;
        }

        QModelIndex sourceParent;
        if (parent.isValid()) {
            sourceParent = mapToSource(parent);
        } else {
            sourceParent = m_rootIndex;
        }

        if (!sourceParent.isValid()) {
            return 0;
        }

        return sourceModel()->columnCount(sourceParent);
    }

    QModelIndex LyricsSubtreeProxyModel::rootIndex() const {
        return m_rootIndex;
    }

    void LyricsSubtreeProxyModel::setRootIndex(const QModelIndex &index) {
        if (m_rootIndex == index) {
            return;
        }

        beginResetModel();
        m_rootIndex = QPersistentModelIndex(index);
        endResetModel();

        emit rootIndexChanged();
    }

    QVariant LyricsSubtreeProxyModel::data(const QModelIndex &index, int role) const {
        if (!sourceModel()) {
            return QVariant();
        }

        QModelIndex sourceIndex = mapToSource(index);
        return sourceModel()->data(sourceIndex, role);
    }

    QVariant LyricsSubtreeProxyModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (!sourceModel()) {
            return QVariant();
        }

        return sourceModel()->headerData(section, orientation, role);
    }

    Qt::ItemFlags LyricsSubtreeProxyModel::flags(const QModelIndex &index) const {
        if (!sourceModel()) {
            return Qt::NoItemFlags;
        }

        QModelIndex sourceIndex = mapToSource(index);
        return sourceModel()->flags(sourceIndex);
    }
    QHash<int, QByteArray> LyricsSubtreeProxyModel::roleNames() const {
        return {
            {USDef::LC_PronunciationRole, "pronunciation"},
            {USDef::LC_LyricRole, "lyric"},
            {USDef::LC_CandidatePronunciationsRole, "candidatePronunciations"}
        };
    }

    void LyricsSubtreeProxyModel::setSourceModel(QAbstractItemModel *sourceModel) {
        if (this->sourceModel()) {
            disconnect(this->sourceModel(), nullptr, this, nullptr);
        }

        QAbstractProxyModel::setSourceModel(sourceModel);

        if (sourceModel) {
            connect(sourceModel, &QAbstractItemModel::dataChanged,
                    this, &LyricsSubtreeProxyModel::sourceDataChanged);
            connect(sourceModel, &QAbstractItemModel::rowsAboutToBeInserted,
                    this, &LyricsSubtreeProxyModel::sourceRowsAboutToBeInserted);
            connect(sourceModel, &QAbstractItemModel::rowsInserted,
                    this, &LyricsSubtreeProxyModel::sourceRowsInserted);
            connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved,
                    this, &LyricsSubtreeProxyModel::sourceRowsAboutToBeRemoved);
            connect(sourceModel, &QAbstractItemModel::rowsRemoved,
                    this, &LyricsSubtreeProxyModel::sourceRowsRemoved);
            connect(sourceModel, &QAbstractItemModel::modelReset,
                    this, &LyricsSubtreeProxyModel::sourceModelReset);
        }

        // Reset the model when source model changes
        beginResetModel();
        endResetModel();
    }

    void LyricsSubtreeProxyModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
        if (!isValidSourceIndex(topLeft) || !isValidSourceIndex(bottomRight)) {
            return;
        }

        QModelIndex proxyTopLeft = mapFromSource(topLeft);
        QModelIndex proxyBottomRight = mapFromSource(bottomRight);

        if (proxyTopLeft.isValid() && proxyBottomRight.isValid()) {
            emit dataChanged(proxyTopLeft, proxyBottomRight, roles);
        }
    }

    void LyricsSubtreeProxyModel::sourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last) {
        if (!isValidSourceIndex(parent) && parent != m_rootIndex) {
            return;
        }

        QModelIndex proxyParent = mapFromSource(parent);
        if (parent == m_rootIndex) {
            proxyParent = QModelIndex(); // Top level in proxy model
        }
        m_doInsert = true;
        beginInsertRows(proxyParent, first, last);
    }

    void LyricsSubtreeProxyModel::sourceRowsInserted(const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        Q_UNUSED(first)
        Q_UNUSED(last)
        if (!m_doInsert) {
            return;
        }
        m_doInsert = false;
        endInsertRows();
    }

    void LyricsSubtreeProxyModel::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last) {
        if (!isValidSourceIndex(parent) && parent != m_rootIndex) {
            return;
        }

        QModelIndex proxyParent = mapFromSource(parent);
        if (parent == m_rootIndex) {
            proxyParent = QModelIndex(); // Top level in proxy model
        }
        m_doRemove = true;
        beginRemoveRows(proxyParent, first, last);
    }

    void LyricsSubtreeProxyModel::sourceRowsRemoved(const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        Q_UNUSED(first)
        Q_UNUSED(last)
        if (!m_doRemove) {
            return;
        }
        m_doRemove = false;
        endRemoveRows();
    }

    void LyricsSubtreeProxyModel::sourceModelReset() {
        beginResetModel();
        endResetModel();
    }

    bool LyricsSubtreeProxyModel::isValidSourceIndex(const QModelIndex &sourceIndex) const {
        if (!sourceIndex.isValid() || !m_rootIndex.isValid()) {
            return false;
        }

        // Check if sourceIndex is a descendant of m_rootIndex
        QModelIndex current = sourceIndex;
        while (current.isValid()) {
            if (current == m_rootIndex) {
                return true;
            }
            current = current.parent();
        }

        return false;
    }

    QModelIndex LyricsSubtreeProxyModel::sourceIndexToProxyIndex(const QModelIndex &sourceIndex) const {
        if (!sourceIndex.isValid() || !m_rootIndex.isValid()) {
            return QModelIndex();
        }

        // Build path from m_rootIndex to sourceIndex
        QList<int> path;
        QModelIndex current = sourceIndex;
        
        while (current.isValid() && current != m_rootIndex) {
            path.prepend(current.row());
            current = current.parent();
        }

        if (current != m_rootIndex) {
            return QModelIndex(); // sourceIndex is not under m_rootIndex
        }

        if (path.isEmpty()) {
            return QModelIndex(); // sourceIndex is m_rootIndex itself
        }

        // Create proxy index with the path information
        SubtreeProxyItemData *itemData = new SubtreeProxyItemData(path, sourceIndex.column());
        int proxyRow = path.last();
        
        return createIndex(proxyRow, sourceIndex.column(), itemData);
    }

    QModelIndex LyricsSubtreeProxyModel::proxyIndexToSourceIndex(const QModelIndex &proxyIndex) const {
        if (!proxyIndex.isValid() || !m_rootIndex.isValid()) {
            return QModelIndex();
        }

        SubtreeProxyItemData *itemData = static_cast<SubtreeProxyItemData*>(proxyIndex.internalPointer());
        if (!itemData) {
            return QModelIndex();
        }

        // Build source index using the stored path
        QModelIndex sourceIndex = m_rootIndex;
        for (int row : itemData->pathFromRoot) {
            sourceIndex = sourceModel()->index(row, itemData->column, sourceIndex);
            if (!sourceIndex.isValid()) {
                return QModelIndex();
            }
        }

        return sourceIndex;
    }

}
