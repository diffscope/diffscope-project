#include "SingingClip.h"

#include <opendspx/singingclip.h>

#include <dspxmodel/BusControl.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/ParamMap.h>
#include <dspxmodel/SourceMap.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class SingingClipPrivate {
        Q_DECLARE_PUBLIC(SingingClip)
    public:
        SingingClip *q_ptr;
        ModelPrivate *pModel;
        NoteSequence *notes;
        ParamMap *params;
        SourceMap *sources;
    };

    SingingClip::SingingClip(Handle handle, Model *model) : Clip(Singing, handle, model), d_ptr(new SingingClipPrivate) {
        Q_D(SingingClip);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_SingingClip);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->notes = d->pModel->createObject<NoteSequence>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Children));
        d->params = d->pModel->createObject<ParamMap>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Params));
        d->sources = d->pModel->createObject<SourceMap>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Sources));
    }

    SingingClip::~SingingClip() = default;

    NoteSequence *SingingClip::notes() const {
        Q_D(const SingingClip);
        return d->notes;
    }

    ParamMap *SingingClip::params() const {
        Q_D(const SingingClip);
        return d->params;
    }

    SourceMap *SingingClip::sources() const {
        Q_D(const SingingClip);
        return d->sources;
    }

    opendspx::SingingClip SingingClip::toOpenDspx() const {
        return {
            name().toStdString(),
            control()->toOpenDspx(),
            time()->toOpenDspx(),
            workspace()->toOpenDspx(),
            notes()->toOpenDspx(),
            params()->toOpenDspx(),
            sources()->toOpenDspx(),
        };
    }

    void SingingClip::fromOpenDspx(const opendspx::SingingClip &clip) {
        setName(QString::fromStdString(clip.name));
        control()->fromOpenDspx(clip.control);
        time()->fromOpenDspx(clip.time);
        workspace()->fromOpenDspx(clip.workspace);
        notes()->fromOpenDspx(clip.notes);
        params()->fromOpenDspx(clip.params);
        sources()->fromOpenDspx(clip.sources);
    }

    void SingingClip::handleSetEntityProperty(int property, const QVariant &value) {
        Clip::handleSetEntityProperty(property, value);
    }

}

#include "moc_SingingClip.cpp"
