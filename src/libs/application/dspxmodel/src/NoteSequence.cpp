#include "NoteSequence.h"
#include "NoteSequence_p.h"

#include <QJSEngine>
#include <QJSValue>

#include <opendspx/note.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    NoteSequence::NoteSequence(SingingClip *singingClip, Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new NoteSequencePrivate) {
        Q_D(NoteSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_Notes);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->singingClip = singingClip;

        d->init(model->strategy()->getEntitiesFromSequenceContainer(handle));
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
