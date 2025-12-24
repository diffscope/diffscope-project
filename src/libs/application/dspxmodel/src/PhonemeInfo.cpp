#include "PhonemeInfo.h"
#include "PhonemeInfo_p.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/phonemes.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/PhonemeList.h>
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
        d->edited = d->pModel->createObject<PhonemeList>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_PhonemesEdited));
        d->original = d->pModel->createObject<PhonemeList>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_PhonemesOriginal));
    }

    PhonemeInfo::~PhonemeInfo() = default;

    PhonemeList *PhonemeInfo::edited() const {
        Q_D(const PhonemeInfo);
        return d->edited;
    }

    PhonemeList *PhonemeInfo::original() const {
        Q_D(const PhonemeInfo);
        return d->original;
    }

    QDspx::Phonemes PhonemeInfo::toQDspx() const {
        return {
            original()->toQDspx(),
            edited()->toQDspx()
        };
    }

    void PhonemeInfo::fromQDspx(const QDspx::Phonemes &phonemeInfo) {
        Q_D(PhonemeInfo);
        original()->fromQDspx(phonemeInfo.original);
        edited()->fromQDspx(phonemeInfo.edited);
    }

    Note *PhonemeInfo::note() const {
        Q_D(const PhonemeInfo);
        return d->note;
    }

}

#include "moc_PhonemeInfo.cpp"
