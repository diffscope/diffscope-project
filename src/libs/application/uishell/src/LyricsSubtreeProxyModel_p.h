#ifndef UISHELL_LYRICSSUBTREEPROXYMODEL_H
#define UISHELL_LYRICSSUBTREEPROXYMODEL_H

#include <QAbstractProxyModel>
#include <QList>
#include <qqmlintegration.h>

namespace UIShell {

    class LyricsSubtreeProxyModel : public QAbstractProxyModel {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    public:
        explicit LyricsSubtreeProxyModel(QObject *parent = nullptr);
        ~LyricsSubtreeProxyModel() override;

        // QAbstractProxyModel interface
        QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
        QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &child) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;

        // Property accessors
        QModelIndex rootIndex() const;
        void setRootIndex(const QModelIndex &index);

        // QAbstractItemModel interface
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;

        QHash<int, QByteArray> roleNames() const override;

    public slots:
        void setSourceModel(QAbstractItemModel *sourceModel) override;

    signals:
        void rootIndexChanged();

    private slots:
        void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
        void sourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
        void sourceRowsInserted(const QModelIndex &parent, int first, int last);
        void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
        void sourceRowsRemoved(const QModelIndex &parent, int first, int last);
        void sourceModelReset();

    private:
        bool isValidSourceIndex(const QModelIndex &sourceIndex) const;
        QModelIndex sourceIndexToProxyIndex(const QModelIndex &sourceIndex) const;
        QModelIndex proxyIndexToSourceIndex(const QModelIndex &proxyIndex) const;
        
        QPersistentModelIndex m_rootIndex;

        bool m_doInsert = false;
        bool m_doRemove = false;
    };

}

#endif //UISHELL_LYRICSSUBTREEPROXYMODEL_H
