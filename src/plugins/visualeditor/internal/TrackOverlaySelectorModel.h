#ifndef DIFFSCOPE_VISUALEDITOR_TRACKOVERLAYSELECTORMODEL_H
#define DIFFSCOPE_VISUALEDITOR_TRACKOVERLAYSELECTORMODEL_H

#include <QObject>
#include <QStandardItemModel>

namespace dspx {
    class Track;
    class TrackList;
}

namespace VisualEditor::Internal {

    class TrackOverlaySelectorItem : public QObject {
        Q_OBJECT
        Q_PROPERTY(dspx::Track *track READ track WRITE setTrack NOTIFY trackChanged)
        Q_PROPERTY(bool overlayVisible READ overlayVisible WRITE setOverlayVisible NOTIFY overlayVisibleChanged)
    public:
        explicit TrackOverlaySelectorItem(QObject *parent = nullptr);

        dspx::Track *track() const;
        void setTrack(dspx::Track *track);

        bool overlayVisible() const;
        void setOverlayVisible(bool overlayVisible);

    Q_SIGNALS:
        void trackChanged();
        void overlayVisibleChanged();

    private:
        dspx::Track *m_track = nullptr;
        bool m_overlayVisible = true;
    };

    class TrackOverlaySelectorModel : public QStandardItemModel {
        Q_OBJECT

        Q_PROPERTY(dspx::TrackList *trackList READ trackList WRITE setTrackList NOTIFY trackListChanged)

    public:
        enum Roles {
            DisplayRole = Qt::DisplayRole,
            TrackRole = Qt::UserRole + 1,
        };
        Q_ENUM(Roles)

        explicit TrackOverlaySelectorModel(QObject *parent = nullptr);
        ~TrackOverlaySelectorModel() override;

        dspx::TrackList *trackList() const;
        void setTrackList(dspx::TrackList *trackList);

        QHash<int, QByteArray> roleNames() const override;

    Q_SIGNALS:
        void trackListChanged();

    private:
        void resetModel();
        void populateFromTrackList();
        void insertTrack(int index, dspx::Track *track);
        void removeTrack(int index, dspx::Track *track);
        void moveTracks(int from, int to, int count);
        int rowForTrack(const dspx::Track *track) const;
        TrackOverlaySelectorItem *createItemObject(dspx::Track *track) const;

    private:
        dspx::TrackList *m_trackList = nullptr;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_TRACKOVERLAYSELECTORMODEL_H
