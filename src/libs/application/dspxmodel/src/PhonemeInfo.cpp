#include "PhonemeInfo.h"
#include "PhonemeInfo_p.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/phonemes.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/PhonemeSequence.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    void PhonemeInfoPrivate::setNote(PhonemeInfo *item, Note *note) {
        auto d = item->d_func();
        if (d->note != note) {
            d->note = note;
            Q_EMIT item->noteChanged();
        }
    }

    PhonemeInfo::PhonemeInfo(Note *note, Handle handle, Model *model) : QObject(model), d_ptr(new PhonemeInfoPrivate) {
        Q_D(PhonemeInfo);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->note = note;
        d->edited = d->pModel->createObject<PhonemeSequence>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_PhonemesEdited));
        d->original = d->pModel->createObject<PhonemeSequence>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_PhonemesOriginal));
    }

    PhonemeInfo::~PhonemeInfo() = default;

    PhonemeSequence *PhonemeInfo::edited() const {
        Q_D(const PhonemeInfo);
        return d->edited;
    }

    PhonemeSequence *PhonemeInfo::original() const {
        Q_D(const PhonemeInfo);
        return d->original;
    }

    opendspx::Phonemes PhonemeInfo::toOpenDspx() const {
        return {
            original()->toOpenDspx(),
            edited()->toOpenDspx()
        };
    }

    void PhonemeInfo::fromOpenDspx(const opendspx::Phonemes &phonemeInfo) {
        Q_D(PhonemeInfo);
        original()->fromOpenDspx(phonemeInfo.original);
        edited()->fromOpenDspx(phonemeInfo.edited);
    }

    Note *PhonemeInfo::note() const {
        Q_D(const PhonemeInfo);
        return d->note;
    }

}

#include "moc_PhonemeInfo.cpp"
