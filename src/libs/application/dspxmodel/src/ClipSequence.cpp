#include "ClipSequence.h"

#include <QJSValue>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/AudioClip.h>
#include <dspxmodel/SingingClip.h>

namespace dspx {

    class ClipSequencePrivate : public PointSequenceData<ClipSequence, Clip, &Clip::position, &Clip::positionChanged> {
        Q_DECLARE_PUBLIC(ClipSequence)
    public:
        Track *track{};
    };

    ClipSequence::ClipSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new ClipSequencePrivate) {
        Q_D(ClipSequence);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        connect(this, &ClipSequence::itemInserted, this, [=](Clip *item) {
            item->setTrack(d->track);
        });
        connect(this, &ClipSequence::itemRemoved, this, [=](Clip *item) {
            item->setTrack(nullptr);
        });
    }

    ClipSequence::~ClipSequence() = default;

    int ClipSequence::size() const {
        Q_D(const ClipSequence);
        return d->container.size();
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
        return d->container.previousItem(item);
    }

    Clip *ClipSequence::nextItem(Clip *item) const {
        Q_D(const ClipSequence);
        return d->container.nextItem(item);
    }

    QList<Clip *> ClipSequence::slice(int position, int length) const {
        Q_D(const ClipSequence);
        return d->container.slice(position, length);
    }

    bool ClipSequence::contains(Clip *item) const {
        Q_D(const ClipSequence);
        return d->container.contains(item);
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
        ret.reserve(d->container.size());
        for (const auto &[_, item] : d->container.m_items) {
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