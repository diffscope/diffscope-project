
#include "SubtreeProxyModel_p.h"

namespace UIShell {

    struct SubtreeProxyItemData {
        QList<int> pathFromRoot; // Path from m_rootIndex to the source item
        int column;

        SubtreeProxyItemData(const QList<int> &path, int col) : pathFromRoot(path), column(col) {}
    };

    SubtreeProxyModel::SubtreeProxyModel(QObject *parent)
        : QAbstractProxyModel(parent) {
    }

    SubtreeProxyModel::~SubtreeProxyModel() {
        // Cleanup will be handled automatically when the model is destroyed
        // since we're using QModelIndex's built-in cleanup mechanism
    }

    QModelIndex SubtreeProxyModel::mapFromSource(const QModelIndex &sourceIndex) const {
        if (!sourceModel() || !sourceIndex.isValid()) {
            return QModelIndex();
        }
        if (!isValidSourceIndex(sourceIndex)) {
            return QModelIndex();
        }
        return sourceIndexToProxyIndex(sourceIndex);
    }

    QModelIndex SubtreeProxyModel::mapToSource(const QModelIndex &proxyIndex) const {
        if (!sourceModel() || !proxyIndex.isValid()) {
            return QModelIndex();
        }
        return proxyIndexToSourceIndex(proxyIndex);
    }

    QModelIndex SubtreeProxyModel::index(int row, int column, const QModelIndex &parent) const {
        if (!sourceModel() || !m_rootIndex.isValid()) {
            return QModelIndex();
        }
        if (row < 0 || column < 0) {
            return QModelIndex();
        }
        QList<int> pathFromRoot;
        QModelIndex sourceParent = m_rootIndex;
        if (parent.isValid()) {
            SubtreeProxyItemData *parentData = static_cast<SubtreeProxyItemData*>(parent.internalPointer());
            if (parentData) {
                pathFromRoot = parentData->pathFromRoot;
                pathFromRoot.append(parent.row());
                sourceParent = m_rootIndex;
                for (int rowInPath : pathFromRoot) {
                    sourceParent = sourceModel()->index(rowInPath, 0, sourceParent);
                    if (!sourceParent.isValid()) {
                        return QModelIndex();
                    }
                }
            }
        }
        QModelIndex sourceIndex = sourceModel()->index(row, column, sourceParent);
        if (!sourceIndex.isValid()) {
            return QModelIndex();
        }
        QList<int> currentPath = pathFromRoot;
        currentPath.append(row);
        SubtreeProxyItemData *itemData = new SubtreeProxyItemData(currentPath, column);
        return createIndex(row, column, itemData);
    }

    QModelIndex SubtreeProxyModel::parent(const QModelIndex &child) const {
        if (!sourceModel() || !child.isValid() || !m_rootIndex.isValid()) {
            return QModelIndex();
        }
        SubtreeProxyItemData *itemData = static_cast<SubtreeProxyItemData*>(child.internalPointer());
        if (!itemData || itemData->pathFromRoot.isEmpty()) {
            return QModelIndex();
        }
        QList<int> parentPath = itemData->pathFromRoot;
        int parentRow = parentPath.takeLast();
        if (parentPath.isEmpty()) {
            SubtreeProxyItemData *parentData = new SubtreeProxyItemData(QList<int>(), itemData->column);
            return createIndex(parentRow, itemData->column, parentData);
        } else {
            SubtreeProxyItemData *parentData = new SubtreeProxyItemData(parentPath, itemData->column);
            return createIndex(parentRow, itemData->column, parentData);
        }
    }

    int SubtreeProxyModel::rowCount(const QModelIndex &parent) const {
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

    int SubtreeProxyModel::columnCount(const QModelIndex &parent) const {
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

    QModelIndex SubtreeProxyModel::rootIndex() const {
        return m_rootIndex;
    }

    void SubtreeProxyModel::setRootIndex(const QModelIndex &index) {
        if (m_rootIndex == index) {
            return;
        }
        beginResetModel();
        m_rootIndex = QPersistentModelIndex(index);
        endResetModel();
        emit rootIndexChanged();
    }

    QVariant SubtreeProxyModel::data(const QModelIndex &index, int role) const {
        if (!sourceModel()) {
            return QVariant();
        }
        QModelIndex sourceIndex = mapToSource(index);
        return sourceModel()->data(sourceIndex, role);
    }

    QVariant SubtreeProxyModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (!sourceModel()) {
            return QVariant();
        }
        return sourceModel()->headerData(section, orientation, role);
    }

    Qt::ItemFlags SubtreeProxyModel::flags(const QModelIndex &index) const {
        if (!sourceModel()) {
            return Qt::NoItemFlags;
        }
        QModelIndex sourceIndex = mapToSource(index);
        return sourceModel()->flags(sourceIndex);
    }

    void SubtreeProxyModel::setSourceModel(QAbstractItemModel *sourceModel) {
        if (this->sourceModel()) {
            disconnect(this->sourceModel(), nullptr, this, nullptr);
        }
        QAbstractProxyModel::setSourceModel(sourceModel);
        if (sourceModel) {
            connect(sourceModel, &QAbstractItemModel::dataChanged,
                    this, &SubtreeProxyModel::sourceDataChanged);
            connect(sourceModel, &QAbstractItemModel::rowsAboutToBeInserted,
                    this, &SubtreeProxyModel::sourceRowsAboutToBeInserted);
            connect(sourceModel, &QAbstractItemModel::rowsInserted,
                    this, &SubtreeProxyModel::sourceRowsInserted);
            connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved,
                    this, &SubtreeProxyModel::sourceRowsAboutToBeRemoved);
            connect(sourceModel, &QAbstractItemModel::rowsRemoved,
                    this, &SubtreeProxyModel::sourceRowsRemoved);
            connect(sourceModel, &QAbstractItemModel::modelReset,
                    this, &SubtreeProxyModel::sourceModelReset);
        }
        beginResetModel();
        endResetModel();
    }

    void SubtreeProxyModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
        if (!isValidSourceIndex(topLeft) || !isValidSourceIndex(bottomRight)) {
            return;
        }
        QModelIndex proxyTopLeft = mapFromSource(topLeft);
        QModelIndex proxyBottomRight = mapFromSource(bottomRight);
        if (proxyTopLeft.isValid() && proxyBottomRight.isValid()) {
            emit dataChanged(proxyTopLeft, proxyBottomRight, roles);
        }
    }

    void SubtreeProxyModel::sourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last) {
        if (!isValidSourceIndex(parent) && parent != m_rootIndex) {
            return;
        }
        QModelIndex proxyParent = mapFromSource(parent);
        if (parent == m_rootIndex) {
            proxyParent = QModelIndex();
        }
        m_doInsert = true;
        beginInsertRows(proxyParent, first, last);
    }

    void SubtreeProxyModel::sourceRowsInserted(const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        Q_UNUSED(first)
        Q_UNUSED(last)
        if (!m_doInsert) {
            return;
        }
        m_doInsert = false;
        endInsertRows();
    }

    void SubtreeProxyModel::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last) {
        if (!isValidSourceIndex(parent) && parent != m_rootIndex) {
            return;
        }
        QModelIndex proxyParent = mapFromSource(parent);
        if (parent == m_rootIndex) {
            proxyParent = QModelIndex();
        }
        m_doRemove = true;
        beginRemoveRows(proxyParent, first, last);
    }

    void SubtreeProxyModel::sourceRowsRemoved(const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        Q_UNUSED(first)
        Q_UNUSED(last)
        if (!m_doRemove) {
            return;
        }
        m_doRemove = false;
        endRemoveRows();
    }

    void SubtreeProxyModel::sourceModelReset() {
        beginResetModel();
        endResetModel();
    }

    bool SubtreeProxyModel::isValidSourceIndex(const QModelIndex &sourceIndex) const {
        if (!sourceIndex.isValid() || !m_rootIndex.isValid()) {
            return false;
        }
        QModelIndex current = sourceIndex;
        while (current.isValid()) {
            if (current == m_rootIndex) {
                return true;
            }
            current = current.parent();
        }
        return false;
    }

    QModelIndex SubtreeProxyModel::sourceIndexToProxyIndex(const QModelIndex &sourceIndex) const {
        if (!sourceIndex.isValid() || !m_rootIndex.isValid()) {
            return QModelIndex();
        }
        QList<int> path;
        QModelIndex current = sourceIndex;
        while (current.isValid() && current != m_rootIndex) {
            path.prepend(current.row());
            current = current.parent();
        }
        if (current != m_rootIndex) {
            return QModelIndex();
        }
        if (path.isEmpty()) {
            return QModelIndex();
        }
        SubtreeProxyItemData *itemData = new SubtreeProxyItemData(path, sourceIndex.column());
        int proxyRow = path.last();
        return createIndex(proxyRow, sourceIndex.column(), itemData);
    }

    QModelIndex SubtreeProxyModel::proxyIndexToSourceIndex(const QModelIndex &proxyIndex) const {
        if (!proxyIndex.isValid() || !m_rootIndex.isValid()) {
            return QModelIndex();
        }
        SubtreeProxyItemData *itemData = static_cast<SubtreeProxyItemData*>(proxyIndex.internalPointer());
        if (!itemData) {
            return QModelIndex();
        }
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
