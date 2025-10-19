#include "ClipSequence.h"

#include <QJSValue>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/RangeSequenceContainer_p.h>
#include <dspxmodel/private/RangeSequenceData_p.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/AudioClip.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/ClipTime.h>

namespace dspx {

    static void setClipOverlapped(Clip *item, bool overlapped);

    class ClipSequencePrivate : public RangeSequenceData<ClipSequence, Clip, &Clip::position, &Clip::positionChanged, &Clip::length, &Clip::lengthChanged, &setClipOverlapped> {
        Q_DECLARE_PUBLIC(ClipSequence)
    public:
        Track *track{};
        static void setOverlapped(Clip *item, bool overlapped) {
            item->setOverlapped(overlapped);
        }
        static void setTrack(Clip *item, Track *track) {
            item->setTrack(track);
        }
    };

    void setClipOverlapped(Clip *item, bool overlapped) {
        ClipSequencePrivate::setOverlapped(item, overlapped);
    }

    ClipSequence::ClipSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new ClipSequencePrivate) {
        Q_D(ClipSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_Clips);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        connect(this, &ClipSequence::itemInserted, this, [=](Clip *item) {
            ClipSequencePrivate::setTrack(item, d->track);
        });
        connect(this, &ClipSequence::itemRemoved, this, [=](Clip *item) {
            ClipSequencePrivate::setTrack(item, nullptr);
        });
    }

    ClipSequence::~ClipSequence() = default;

    int ClipSequence::size() const {
        Q_D(const ClipSequence);
        return d->pointContainer.size();
    }

    Clip *ClipSequence::firstItem() const {
        Q_D(const ClipSequence);
        return d->firstItem;
    }

    Clip *ClipSequence::lastItem() const {
        Q_D(const ClipSequence);
        return d->lastItem;
    }

    Track *ClipSequence::track() const {
        Q_D(const ClipSequence);
        return d->track;
    }

    Clip *ClipSequence::previousItem(Clip *item) const {
        Q_D(const ClipSequence);
        return d->pointContainer.previousItem(item);
    }

    Clip *ClipSequence::nextItem(Clip *item) const {
        Q_D(const ClipSequence);
        return d->pointContainer.nextItem(item);
    }

    QList<Clip *> ClipSequence::slice(int position, int length) const {
        Q_D(const ClipSequence);
        return d->rangeContainer.slice(position, length);
    }

    bool ClipSequence::contains(Clip *item) const {
        Q_D(const ClipSequence);
        return d->pointContainer.contains(item);
    }

    bool ClipSequence::insertItem(Clip *item) {
        Q_D(ClipSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool ClipSequence::removeItem(Clip *item) {
        Q_D(ClipSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    QList<QDspx::ClipRef> ClipSequence::toQDspx() const {
        Q_D(const ClipSequence);
        QList<QDspx::ClipRef> ret;
        ret.reserve(d->pointContainer.size());
        for (const auto &[_, item] : d->pointContainer.m_items) {
            ret.append(item->toQDspx());
        }
        return ret;
    }

    void ClipSequence::fromQDspx(const QList<QDspx::ClipRef> &clips) {
        while (size()) {
            removeItem(firstItem());
        }
        for (const auto &clip : clips) {
            Clip *item = nullptr;
            if (clip->type == QDspx::Clip::Audio) {
                item = model()->createAudioClip();
            } else if (clip->type == QDspx::Clip::Singing) {
                item = model()->createSingingClip();
            }
            if (item) {
                item->fromQDspx(clip);
                insertItem(item);
            }
        }
    }

    void ClipSequence::setTrack(Track *track) {
        Q_D(ClipSequence);
        d->track = track;
    }

    void ClipSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(ClipSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void ClipSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(ClipSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

}

#include "moc_ClipSequence.cpp"