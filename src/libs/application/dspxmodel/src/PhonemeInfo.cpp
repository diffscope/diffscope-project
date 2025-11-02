#include "PhonemeInfo.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/phonemes.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/PhonemeList.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class PhonemeInfoPrivate {
        Q_DECLARE_PUBLIC(PhonemeInfo)
    public:
        PhonemeInfo *q_ptr;
        ModelPrivate *pModel;
        PhonemeList *edited;
        PhonemeList *original;
    };

    PhonemeInfo::PhonemeInfo(Handle handle, Model *model) : QObject(model), d_ptr(new PhonemeInfoPrivate) {
        Q_D(PhonemeInfo);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->edited = d->pModel->createObject<PhonemeList>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_PhonemesEdited));
        d->original = d->pModel->createObject<PhonemeList>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_PhonemesOriginal));
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

}

#include "moc_PhonemeInfo.cpp"
