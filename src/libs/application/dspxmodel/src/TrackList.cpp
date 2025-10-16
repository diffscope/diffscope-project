#include "TrackList.h"

#include "ModelStrategy.h"

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/ListData_p.h>
#include <dspxmodel/Track.h>

namespace dspx {

    class TrackListPrivate : public ListData<TrackList, Track> {
        Q_DECLARE_PUBLIC(TrackList)
    };

    TrackList::TrackList(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new TrackListPrivate) {
        Q_D(TrackList);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    TrackList::~TrackList() = default;

    int TrackList::size() const {
        Q_D(const TrackList);
        return d->size();
    }

    QList<Track *> TrackList::items() const {
        Q_D(const TrackList);
        return d->items();
    }

    bool TrackList::insertItem(int index, Track *item) {
        Q_D(TrackList);
        return d->pModel->strategy->insertIntoListContainer(handle(), item->handle(), index);
    }

    Track *TrackList::removeItem(int index) {
        Q_D(TrackList);
        auto takenEntityHandle = d->pModel->strategy->takeFromListContainer(handle(), index);
        return d->getItem(takenEntityHandle, false);
    }

    Track *TrackList::item(int index) const {
        Q_D(const TrackList);
        return d->item(index);
    }

    bool TrackList::rotate(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(TrackList);
        return d->pModel->strategy->rotateListContainer(handle(), leftIndex, middleIndex, rightIndex);
    }

    QList<QDspx::Track> TrackList::toQDspx() const {
        Q_D(const TrackList);
        QList<QDspx::Track> ret;
        for (const auto &track : d->itemList) {
            ret.append(track->toQDspx());
        }
        return ret;
    }

    void TrackList::fromQDspx(const QList<QDspx::Track> &trackList) {
        while (size() > 0) {
            removeItem(0);
        }
        for (const auto &trackData : trackList) {
            auto track = model()->createTrack();
            track->fromQDspx(trackData);
            insertItem(size(), track);
        }
    }

    void TrackList::handleInsertIntoListContainer(Handle entity, int index) {
        Q_D(TrackList);
        d->handleInsertIntoListContainer(entity, index);
    }

    void TrackList::handleTakeFromListContainer(Handle takenEntity, int index) {
        Q_D(TrackList);
        d->handleTakeFromListContainer(takenEntity, index);
    }

    void TrackList::handleRotateListContainer(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(TrackList);
        d->handleRotateListContainer(leftIndex, middleIndex, rightIndex);
    }

}

#include "moc_TrackList.cpp"