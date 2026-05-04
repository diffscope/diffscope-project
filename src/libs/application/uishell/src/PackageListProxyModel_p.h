#ifndef UISHELL_PACKAGELISTPROXYMODEL_P_H
#define UISHELL_PACKAGELISTPROXYMODEL_P_H

#include <qqmlintegration.h>

#include <QIdentityProxyModel>
#include <QString>

namespace UIShell {

    class PackageListProxyModel : public QIdentityProxyModel {
        Q_OBJECT
        QML_NAMED_ELEMENT(PackageListProxyModel)
    public:
        explicit PackageListProxyModel(QObject *parent = nullptr);
        ~PackageListProxyModel() override;

        QHash<int, QByteArray> roleNames() const override;

        Q_INVOKABLE static QModelIndex invalidIndex() {
            return QModelIndex();
        }
        Q_INVOKABLE QModelIndex packageModelIndex(int index) const;

        Q_INVOKABLE QModelIndex entryIndex(const QModelIndex &index) const;
        Q_INVOKABLE QModelIndex singerModelIndexForIndex(const QModelIndex &index) const;

        Q_INVOKABLE QModelIndex packageModelIndexForIndex(const QModelIndex &index) const;
        Q_INVOKABLE QModelIndex dependencyRootIndexForIndex(const QModelIndex &index) const;
        Q_INVOKABLE QModelIndex singerRootIndexForIndex(const QModelIndex &index) const;
        Q_INVOKABLE QModelIndex inferenceRootIndexForIndex(const QModelIndex &index) const;
        Q_INVOKABLE QModelIndex importRootIndexForSingerIndex(const QModelIndex &index) const;
        Q_INVOKABLE QModelIndex demoAudioRootIndexForSingerIndex(const QModelIndex &index) const;

        Q_INVOKABLE bool isSingerOrSingerRootIndex(const QModelIndex &index) const;
        Q_INVOKABLE bool isInferenceOrInferenceRootIndex(const QModelIndex &index) const;
        Q_INVOKABLE bool isPackageIndex(const QModelIndex &index) const;
        Q_INVOKABLE bool isDependencyRootIndex(const QModelIndex &index) const;

        Q_INVOKABLE int findPackageIndex(const QString &id, const QString &version) const;
        Q_INVOKABLE QModelIndex findInferenceIndex(const QString &id, const QString &version, const QString &inferenceId) const;

        Q_INVOKABLE int packageIndexForIndex(const QModelIndex &index) const;

    private:
        bool isSingerIndex(const QModelIndex &index) const;
        bool isInferenceIndex(const QModelIndex &index) const;
    };

}

#endif // UISHELL_PACKAGELISTPROXYMODEL_P_H
