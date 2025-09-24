#ifndef DIFFSCOPE_COREPLUGIN_FINDACTIONSMODEL_H
#define DIFFSCOPE_COREPLUGIN_FINDACTIONSMODEL_H

#include <QAbstractItemModel>
#include <QCollator>
#include <QStringList>

namespace QAK {
    class QuickActionContext;
}

namespace Core::Internal {

    class FindActionsAddOn;

    class FindActionsModel : public QAbstractItemModel {
        Q_OBJECT
    public:
        explicit FindActionsModel(QAK::QuickActionContext *actionContext, FindActionsAddOn *parent = nullptr);

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
        QObject *getActionObject(const QString &id);

        QAK::QuickActionContext *m_actionContext;
        QStringList m_actions;
        QStringList m_priorityActions;
        QList<QPair<QString, int>> m_actionList;
        QCollator m_collator;
        QHash<QString, QPointer<QObject>> m_actionObjects;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_FINDACTIONSMODEL_H
