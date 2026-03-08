#ifndef DIFFSCOPE_COREPLUGIN_CLIPPROPERTYMAPPER_H
#define DIFFSCOPE_COREPLUGIN_CLIPPROPERTYMAPPER_H

#include <QObject>
#include <qqmlintegration.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {
    class ClipPropertyMapperPrivate;

    class ClipPropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ClipPropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(QVariant type READ type NOTIFY typeChanged)
        Q_PROPERTY(QVariant associatedTrack READ associatedTrack WRITE setAssociatedTrack NOTIFY associatedTrackChanged)
        Q_PROPERTY(QVariant mute READ mute WRITE setMute NOTIFY muteChanged)
        Q_PROPERTY(QVariant gain READ gain WRITE setGain NOTIFY gainChanged)
        Q_PROPERTY(QVariant pan READ pan WRITE setPan NOTIFY panChanged)
        Q_PROPERTY(QVariant position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(QVariant startingOffset READ startingOffset WRITE setStartingOffset NOTIFY startingOffsetChanged)
        Q_PROPERTY(QVariant clipLength READ clipLength WRITE setClipLength NOTIFY clipLengthChanged)
        Q_PROPERTY(QVariant fullLength READ fullLength WRITE setFullLength NOTIFY fullLengthChanged)
    public:
        explicit ClipPropertyMapper(QObject *parent = nullptr);
        ~ClipPropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant name() const;
        void setName(const QVariant &name);

        QVariant type() const;

        QVariant associatedTrack() const;
        void setAssociatedTrack(const QVariant &associatedTrack);

        QVariant mute() const;
        void setMute(const QVariant &mute);

        QVariant gain() const;
        void setGain(const QVariant &gain);

        QVariant pan() const;
        void setPan(const QVariant &pan);

        QVariant position() const;
        void setPosition(const QVariant &position);

        QVariant startingOffset() const;
        void setStartingOffset(const QVariant &startingOffset);

        QVariant clipLength() const;
        void setClipLength(const QVariant &clipLength);

        QVariant fullLength() const;
        void setFullLength(const QVariant &fullLength);

    Q_SIGNALS:
        void selectionModelChanged();
        void nameChanged();
        void typeChanged();
        void associatedTrackChanged();
        void muteChanged();
        void gainChanged();
        void panChanged();
        void positionChanged();
        void startingOffsetChanged();
        void clipLengthChanged();
        void fullLengthChanged();

    private:
        QScopedPointer<ClipPropertyMapperPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_CLIPPROPERTYMAPPER_H
