#include "SingingClip.h"

#include <opendspx/singingclip.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/BusControl.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/NoteSequence.h>

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
        d->notes = d->pModel->createObject<NoteSequence>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Children));
        d->notes->setSingingClip(this);
        // TODO: Initialize params, sources when classes are implemented
        d->params = nullptr;
        d->sources = nullptr;
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

    QDspx::SingingClip SingingClip::toQDspx() const {
        return {
            name(),
            control()->toQDspx(),
            time()->toQDspx(),
            workspace()->toQDspx(),
            notes()->toQDspx(),
            // TODO
            // params()->toQDspx(),
            // sources()->toQDspx(),
        };
    }

    void SingingClip::fromQDspx(const QDspx::SingingClip &clip) {
        setName(clip.name);
        control()->fromQDspx(clip.control);
        time()->fromQDspx(clip.time);
        workspace()->fromQDspx(clip.workspace);
        notes()->fromQDspx(clip.notes);
        // TODO
        // params()->fromQDspx(clip.params);
        // sources()->fromQDspx(clip.sources);
    }

    void SingingClip::handleSetEntityProperty(int property, const QVariant &value) {
        Clip::handleSetEntityProperty(property, value);
    }

}

#include "moc_SingingClip.cpp"