#include "SingingClip.h"

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/BusControl.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/Workspace.h>

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
        // TODO: Initialize notes, params, sources when classes are implemented
        d->notes = nullptr;
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
        QDspx::SingingClip clip;
        clip.control = control()->toQDspx();
        clip.name = name();
        // TODO: Add notes, params, sources serialization when classes are implemented
        clip.time = time()->toQDspx();
        clip.type = QDspx::Clip::Singing;
        clip.workspace = workspace()->toQDspx();
        return clip;
    }

    void SingingClip::fromQDspx(const QDspx::SingingClip &clip) {
        control()->fromQDspx(clip.control);
        setName(clip.name);
        // TODO: Add notes, params, sources deserialization when classes are implemented
        time()->fromQDspx(clip.time);
        workspace()->fromQDspx(clip.workspace);
    }

    void SingingClip::handleSetEntityProperty(int property, const QVariant &value) {
        Clip::handleSetEntityProperty(property, value);
    }

}