#include "PhonemeInfo.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/PhonemeList.h>

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

    QDspx::PhonemeInfo PhonemeInfo::toQDspx() const {
        return {
            original()->toQDspx(),
            edited()->toQDspx()
        };
    }

    void PhonemeInfo::fromQDspx(const QDspx::PhonemeInfo &phonemeInfo) {
        Q_D(PhonemeInfo);
        edited()->fromQDspx(phonemeInfo.edited);
        original()->fromQDspx(phonemeInfo.org);
    }

}

#include "moc_PhonemeInfo.cpp"