#include "VibratoPoints.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/vibratopoints.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/VibratoPointDataArray.h>

namespace dspx {

    class VibratoPointsPrivate {
        Q_DECLARE_PUBLIC(VibratoPoints)
    public:
        VibratoPoints *q_ptr;
        ModelPrivate *pModel;
        VibratoPointDataArray *amp;
        VibratoPointDataArray *freq;
    };

    VibratoPoints::VibratoPoints(Handle handle, Model *model) : QObject(model), d_ptr(new VibratoPointsPrivate) {
        Q_D(VibratoPoints);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->amp = d->pModel->createObject<VibratoPointDataArray>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_VibratoPointsAmplitude));
        d->freq = d->pModel->createObject<VibratoPointDataArray>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_VibratoPointsFrequency));
    }

    VibratoPoints::~VibratoPoints() = default;

    VibratoPointDataArray *VibratoPoints::amp() const {
        Q_D(const VibratoPoints);
        return d->amp;
    }

    VibratoPointDataArray *VibratoPoints::freq() const {
        Q_D(const VibratoPoints);
        return d->freq;
    }

    QDspx::VibratoPoints VibratoPoints::toQDspx() const {
        return {
            .amp = amp()->toQDspx(),
            .freq = freq()->toQDspx(),
        };
    }

    void VibratoPoints::fromQDspx(const QDspx::VibratoPoints &vibratoPoints) {
        Q_D(VibratoPoints);
        amp()->fromQDspx(vibratoPoints.amp);
        freq()->fromQDspx(vibratoPoints.freq);
    }


}

#include "moc_VibratoPoints.cpp"