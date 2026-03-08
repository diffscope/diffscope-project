#include "TrackOverlaySelectorModel.h"

#include <QStandardItem>
#include <QVariant>

#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>

namespace VisualEditor::Internal {

    TrackOverlaySelectorItem::TrackOverlaySelectorItem(QObject *parent) : QObject(parent) {
    }

    dspx::Track *TrackOverlaySelectorItem::track() const {
        return m_track;
    }

    void TrackOverlaySelectorItem::setTrack(dspx::Track *track) {
        if (m_track == track)
            return;
        m_track = track;
        Q_EMIT trackChanged();
    }

    bool TrackOverlaySelectorItem::overlayVisible() const {
        return m_overlayVisible;
    }

    void TrackOverlaySelectorItem::setOverlayVisible(bool overlayVisible) {
        if (m_overlayVisible == overlayVisible)
            return;
        m_overlayVisible = overlayVisible;
        Q_EMIT overlayVisibleChanged();
    }

    TrackOverlaySelectorModel::TrackOverlaySelectorModel(QObject *parent) : QStandardItemModel(parent) {
    }

    TrackOverlaySelectorModel::~TrackOverlaySelectorModel() {
        resetModel();
    }

    dspx::TrackList *TrackOverlaySelectorModel::trackList() const {
        return m_trackList;
    }

    void TrackOverlaySelectorModel::setTrackList(dspx::TrackList *trackList) {
        if (m_trackList == trackList)
            return;

        resetModel();

        m_trackList = trackList;

        if (m_trackList) {
            connect(m_trackList, &dspx::TrackList::itemInserted, this, [this](int index, dspx::Track *track) {
                insertTrack(index, track);
            });
            connect(m_trackList, &dspx::TrackList::itemRemoved, this, [this](int index, dspx::Track *track) {
                removeTrack(index, track);
            });
            connect(m_trackList, &dspx::TrackList::rotated, this, [this](int leftIndex, int middleIndex, int rightIndex) {
                const int from = middleIndex;
                const int count = rightIndex - middleIndex;
                moveTracks(from, leftIndex, count);
            });

            populateFromTrackList();
        }

        Q_EMIT trackListChanged();
    }

    QHash<int, QByteArray> TrackOverlaySelectorModel::roleNames() const {
        static const QHash<int, QByteArray> roles{{Qt::DisplayRole, "display"}, {TrackRole, "track"}};
        return roles;
    }

    void TrackOverlaySelectorModel::resetModel() {
        for (int row = 0; row < rowCount(); ++row) {
            if (auto *item = this->item(row)) {
                auto object = item->data(Qt::DisplayRole).value<QObject *>();
                if (object) {
                    object->deleteLater();
                }
            }
        }

        clear();

        if (m_trackList) {
            disconnect(m_trackList, nullptr, this, nullptr);
        }
    }

    void TrackOverlaySelectorModel::populateFromTrackList() {
        if (!m_trackList)
            return;

        const auto tracks = m_trackList->items();
        for (int i = 0; i < tracks.size(); ++i) {
            insertTrack(i, tracks.at(i));
        }
    }

    void TrackOverlaySelectorModel::insertTrack(int index, dspx::Track *track) {
        if (!track)
            return;

        auto *itemObject = createItemObject(track);

        auto *item = new QStandardItem;
        item->setData(QVariant::fromValue(static_cast<QObject *>(itemObject)), Qt::DisplayRole);
        item->setData(QVariant::fromValue(track), TrackRole);
        insertRow(index, item);
    }

    void TrackOverlaySelectorModel::removeTrack(int index, dspx::Track *track) {
        int row = index;
        if (row < 0 || row >= rowCount()) {
            row = rowForTrack(track);
        } else {
            auto *item = this->item(row);
            if (!item || item->data(TrackRole).value<dspx::Track *>() != track) {
                row = rowForTrack(track);
            }
        }

        if (row < 0 || row >= rowCount())
            return;

        if (auto *item = this->item(row)) {
            auto object = item->data(Qt::DisplayRole).value<QObject *>();
            if (object) {
                object->deleteLater();
            }
        }

        removeRow(row);
    }

    void TrackOverlaySelectorModel::moveTracks(int from, int to, int count) {
        if (count <= 0)
            return;

        if (to > from) {
            to -= count;
        }

        if (from == to)
            return;

        moveRows(QModelIndex(), from, count, QModelIndex(), to);
    }

    int TrackOverlaySelectorModel::rowForTrack(const dspx::Track *track) const {
        if (!track)
            return -1;

        for (int row = 0; row < rowCount(); ++row) {
            auto *item = this->item(row);
            if (!item)
                continue;
            auto storedTrack = item->data(TrackRole).value<dspx::Track *>();
            if (storedTrack == track)
                return row;
        }

        return -1;
    }

    TrackOverlaySelectorItem *TrackOverlaySelectorModel::createItemObject(dspx::Track *track) const {
        auto *object = new TrackOverlaySelectorItem;
        object->setParent(const_cast<TrackOverlaySelectorModel *>(this));
        object->setTrack(track);
        object->setOverlayVisible(true);
        return object;
    }

}

#include "moc_TrackOverlaySelectorModel.cpp"
