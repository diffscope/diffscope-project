#include "NoteSequence.h"

#include <QJSValue>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/RangeSequenceContainer_p.h>
#include <dspxmodel/private/RangeSequenceData_p.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    static void setNoteOverlapped(Note *item, bool overlapped);

    class NoteSequencePrivate : public RangeSequenceData<NoteSequence, Note, &Note::pos, &Note::posChanged, &Note::length, &Note::lengthChanged, &setNoteOverlapped> {
        Q_DECLARE_PUBLIC(NoteSequence)
    public:
        SingingClip *singingClip{};
        static void setOverlapped(Note *item, bool overlapped) {
            item->setOverlapped(overlapped);
        }
        static void setSingingClip(Note *item, SingingClip *singingClip) {
            item->setSingingClip(singingClip);
        }
    };

    void setNoteOverlapped(Note *item, bool overlapped) {
        NoteSequencePrivate::setOverlapped(item, overlapped);
    }

    NoteSequence::NoteSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new NoteSequencePrivate) {
        Q_D(NoteSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_Notes);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        connect(this, &NoteSequence::itemInserted, this, [=](Note *item) {
            NoteSequencePrivate::setSingingClip(item, d->singingClip);
        });
        connect(this, &NoteSequence::itemRemoved, this, [=](Note *item) {
            NoteSequencePrivate::setSingingClip(item, nullptr);
        });
    }

    NoteSequence::~NoteSequence() = default;

    int NoteSequence::size() const {
        Q_D(const NoteSequence);
        return d->pointContainer.size();
    }

    Note *NoteSequence::firstItem() const {
        Q_D(const NoteSequence);
        return d->firstItem;
    }

    Note *NoteSequence::lastItem() const {
        Q_D(const NoteSequence);
        return d->lastItem;
    }

    SingingClip *NoteSequence::singingClip() const {
        Q_D(const NoteSequence);
        return d->singingClip;
    }

    Note *NoteSequence::previousItem(Note *item) const {
        Q_D(const NoteSequence);
        return d->pointContainer.previousItem(item);
    }

    Note *NoteSequence::nextItem(Note *item) const {
        Q_D(const NoteSequence);
        return d->pointContainer.nextItem(item);
    }

    QList<Note *> NoteSequence::slice(int position, int length) const {
        Q_D(const NoteSequence);
        return d->pointContainer.slice(position, length);
    }

    bool NoteSequence::contains(Note *item) const {
        Q_D(const NoteSequence);
        return d->pointContainer.contains(item);
    }

    bool NoteSequence::insertItem(Note *item) {
        Q_D(NoteSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool NoteSequence::removeItem(Note *item) {
        Q_D(NoteSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    QList<QDspx::Note> NoteSequence::toQDspx() const {
        Q_D(const NoteSequence);
        QList<QDspx::Note> ret;
        for (const auto &[_, item] : d->pointContainer.m_items) {
            ret.append(item->toQDspx());
        }
        return ret;
    }

    void NoteSequence::fromQDspx(const QList<QDspx::Note> &notes) {
        while (size() > 0) {
            removeItem(firstItem());
        }
        for (const auto &noteData : notes) {
            auto note = model()->createNote();
            note->fromQDspx(noteData);
            insertItem(note);
        }
    }

    void NoteSequence::setSingingClip(SingingClip *singingClip) {
        Q_D(NoteSequence);
        d->singingClip = singingClip;
    }

    void NoteSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(NoteSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void NoteSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(NoteSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

}

#include "moc_NoteSequence.cpp"