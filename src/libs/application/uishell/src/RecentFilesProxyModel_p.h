#ifndef UISHELL_RECENTFILESPROXYMODEL_P_H
#define UISHELL_RECENTFILESPROXYMODEL_P_H

#include <qqmlintegration.h>

#include <QSortFilterProxyModel>

namespace UIShell {

    class RecentFilesProxyModel : public QSortFilterProxyModel {
        Q_OBJECT
        QML_NAMED_ELEMENT(RecentFilesProxyModel)
        Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    public:
        explicit RecentFilesProxyModel(QObject *parent = nullptr);
        ~RecentFilesProxyModel() override;

        QHash<int, QByteArray> roleNames() const override;

        Q_INVOKABLE inline int mapIndexToSource(int i) const {
            return mapToSource(index(i, 0)).row();
        }

    signals:
        void countChanged();
    };

}

#endif //UISHELL_RECENTFILESPROXYMODEL_P_H
