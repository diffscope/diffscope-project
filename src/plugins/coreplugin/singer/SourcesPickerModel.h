#ifndef DIFFSCOPE_COREPLUGIN_SOURCESPICKERMODEL_H
#define DIFFSCOPE_COREPLUGIN_SOURCESPICKERMODEL_H

#include <QAbstractItemModel>
#include <QByteArray>
#include <QJsonValue>
#include <QList>
#include <QScopedPointer>
#include <QString>
#include <QVariantList>
#include <qqmlintegration.h>

#include <opendspx/singer.h>

#include <coreplugin/coreglobal.h>

namespace dspx {
    class Sources;
}

namespace Core {

    class SourcesPickerModelPrivate;

    class CORE_EXPORT SourcesPickerModel : public QAbstractItemModel {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(SourcesPickerModel)
        Q_PROPERTY(QString architectureId READ architectureId WRITE setArchitectureId NOTIFY architectureIdChanged)
        Q_PROPERTY(int count READ count NOTIFY countChanged)
        Q_PROPERTY(bool empty READ empty NOTIFY emptyChanged)
        Q_PROPERTY(bool valid READ valid NOTIFY validationChanged)
        Q_PROPERTY(bool architectureExists READ architectureExists NOTIFY validationChanged)
        Q_PROPERTY(bool canAppend READ canAppend NOTIFY validationChanged)
        Q_PROPERTY(QString mixGroup READ mixGroup NOTIFY validationChanged)
        Q_PROPERTY(QVariantList validationIssues READ validationIssues NOTIFY validationChanged)
        Q_PROPERTY(qulonglong revision READ revision NOTIFY revisionChanged)

    public:
        enum SingerType {
            InvalidSinger = -1,
            SingleSinger,
            MixedSinger,
        };
        Q_ENUM(SingerType)

        enum Role {
            SingerTypeRole = Qt::UserRole + 1,
            SingerIdRole,
            ExtraRole,
            SingerTreeRole,
            WorkspaceNameRole,
            DisplayNameRole,
            RatioRole,
            SingerValidRole,
            WarningTextRole,
            EffectiveMixGroupRole,
            ModelIndexRole,
        };
        Q_ENUM(Role)

        explicit SourcesPickerModel(QObject *parent = nullptr);
        ~SourcesPickerModel() override;

        QString architectureId() const;
        void setArchitectureId(const QString &architectureId);

        QList<opendspx::SingerRef> singers() const;
        void setSingers(const QList<opendspx::SingerRef> &singers);
        Q_INVOKABLE void fromSources(dspx::Sources *sources);
        Q_INVOKABLE void fromDefaultSinger(const QString &singerId);
        Q_INVOKABLE QByteArray serialize() const;
        Q_INVOKABLE bool deserialize(const QByteArray &data);

        int count() const;
        bool empty() const;
        bool valid() const;
        bool architectureExists() const;
        bool canAppend() const;
        QString mixGroup() const;
        QVariantList validationIssues() const;
        qulonglong revision() const;

        QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
        QModelIndex parent(const QModelIndex &child) const override;
        int rowCount(const QModelIndex &parent = {}) const override;
        int columnCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QHash<int, QByteArray> roleNames() const override;

        Q_INVOKABLE QModelIndex modelIndex(int row, const QModelIndex &parent = {}) const;
        Q_INVOKABLE bool indexAlive(const QModelIndex &index) const;
        Q_INVOKABLE int childCount(const QModelIndex &parent = {}) const;
        Q_INVOKABLE SingerType singerType(const QModelIndex &index) const;
        Q_INVOKABLE QString singerId(const QModelIndex &index) const;
        Q_INVOKABLE QJsonValue singerExtra(const QModelIndex &index) const;
        Q_INVOKABLE QString workspaceName(const QModelIndex &index) const;
        Q_INVOKABLE QString displayName(const QModelIndex &index) const;
        Q_INVOKABLE bool singerValid(const QModelIndex &index) const;
        Q_INVOKABLE QString warningText(const QModelIndex &index) const;
        Q_INVOKABLE QString effectiveMixGroup(const QModelIndex &index) const;
        Q_INVOKABLE QString firstLeafSingerId(const QModelIndex &index = {}) const;
        Q_INVOKABLE QVariantList ratios(const QModelIndex &mixedSingerIndex) const;

        Q_INVOKABLE bool trySelectInitialSinger(const QString &architectureId, const QString &singerId);
        Q_INVOKABLE bool tryAppendSinger(const QString &architectureId, const QString &singerId);
        Q_INVOKABLE bool tryAppendSingerToMixed(const QModelIndex &mixedSingerIndex, const QString &architectureId, const QString &singerId);
        Q_INVOKABLE bool tryReplaceSinger(const QModelIndex &index, const QString &architectureId, const QString &singerId);
        Q_INVOKABLE bool removeSinger(const QModelIndex &index);
        Q_INVOKABLE bool moveSinger(const QModelIndex &index, int destinationRow);
        Q_INVOKABLE bool wrapSingerInMixed(const QModelIndex &index);
        Q_INVOKABLE bool setSingerExtra(const QModelIndex &index, const QJsonValue &extra);
        Q_INVOKABLE bool setMixedName(const QModelIndex &index, const QString &name);
        Q_INVOKABLE bool setSingerRatio(const QModelIndex &mixedSingerIndex, int row, double ratio);
        Q_INVOKABLE bool setAdjacentRatios(const QModelIndex &mixedSingerIndex, int leftRow, double leftRatio);

    Q_SIGNALS:
        void architectureIdChanged(const QString &architectureId);
        void singersChanged();
        void countChanged(int count);
        void emptyChanged(bool empty);
        void validationChanged();
        void revisionChanged(qulonglong revision);
        void operationRejected(const QString &message);

    private:
        QScopedPointer<SourcesPickerModelPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_SOURCESPICKERMODEL_H
