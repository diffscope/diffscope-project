#ifndef DIFFSCOPE_COREPLUGIN_FINDACTIONSMODEL_H
#define DIFFSCOPE_COREPLUGIN_FINDACTIONSMODEL_H

#include <QAbstractItemModel>
#include <QCollator>
#include <QStringList>

namespace Core::Internal {

    class FindActionsModel : public QAbstractItemModel {
        Q_OBJECT
    public:
        explicit FindActionsModel(QObject *parent = nullptr);

        // QAbstractItemModel interface
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &child) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void setActions(const QStringList &actions);
        void setPriorityActions(const QStringList &priorityActions);
        void refresh();

    private:
        void updateActionList();

        QStringList m_actions;
        QStringList m_priorityActions;
        QStringList m_actionList; // Combined and sorted list for display
        QCollator m_collator;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_FINDACTIONSMODEL_H
