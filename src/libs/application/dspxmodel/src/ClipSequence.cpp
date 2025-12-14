#include "ClipSequence.h"
#include "ClipSequence_p.h"

#include <QJSEngine>
#include <QJSValue>

#include <opendspx/clip.h>

#include <dspxmodel/AudioClip.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    ClipSequence::ClipSequence(Track *track, Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new ClipSequencePrivate) {
        Q_D(ClipSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_Clips);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->track = track;

        d->init(model->strategy()->getEntitiesFromSequenceContainer(handle));

        connect(this, &ClipSequence::itemInserted, this, [=](Clip *item) {
            ClipPrivate::setClipSequence(item, this);
        });
        connect(this, &ClipSequence::itemRemoved, this, [=](Clip *item) {
            ClipPrivate::setClipSequence(item, nullptr);
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
            switch (clip->type) {
                case QDspx::Clip::Audio:
                    item = model()->createAudioClip();
                    break;
                case QDspx::Clip::Singing:
                    item = model()->createSingingClip();
                    break;
                default:
                    Q_UNREACHABLE();
            }
            item->fromQDspx(clip);
            insertItem(item);
        }
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
