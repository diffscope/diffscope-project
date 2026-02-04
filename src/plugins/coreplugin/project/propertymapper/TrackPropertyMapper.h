#ifndef DIFFSCOPE_COREPLUGIN_TRACKPROPERTYMAPPER_H
#define DIFFSCOPE_COREPLUGIN_TRACKPROPERTYMAPPER_H

#include <QObject>
#include <QVariant>
#include <QScopedPointer>
#include <qqmlintegration.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {
    class TrackPropertyMapperPrivate;

    class TrackPropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(TrackPropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(QVariant colorId READ colorId WRITE setColorId NOTIFY colorIdChanged)
        Q_PROPERTY(QVariant height READ height WRITE setHeight NOTIFY heightChanged)
        Q_PROPERTY(QVariant mute READ mute WRITE setMute NOTIFY muteChanged)
        Q_PROPERTY(QVariant solo READ solo WRITE setSolo NOTIFY soloChanged)
        Q_PROPERTY(QVariant record READ record WRITE setRecord NOTIFY recordChanged)
        Q_PROPERTY(QVariant gain READ gain WRITE setGain NOTIFY gainChanged)
        Q_PROPERTY(QVariant pan READ pan WRITE setPan NOTIFY panChanged)

    public:
        explicit TrackPropertyMapper(QObject *parent = nullptr);
        ~TrackPropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant name() const;
        void setName(const QVariant &name);

        QVariant colorId() const;
        void setColorId(const QVariant &colorId);

        QVariant height() const;
        void setHeight(const QVariant &height);

        QVariant mute() const;
        void setMute(const QVariant &mute);

        QVariant solo() const;
        void setSolo(const QVariant &solo);

        QVariant record() const;
        void setRecord(const QVariant &record);

        QVariant gain() const;
        void setGain(const QVariant &gain);

        QVariant pan() const;
        void setPan(const QVariant &pan);

    Q_SIGNALS:
        void selectionModelChanged();
        void nameChanged();
        void colorIdChanged();
        void heightChanged();
        void muteChanged();
        void soloChanged();
        void recordChanged();
        void gainChanged();
        void panChanged();

    private:
        QScopedPointer<TrackPropertyMapperPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_TRACKPROPERTYMAPPER_H
