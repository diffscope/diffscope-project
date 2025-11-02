#include "AudioClip.h"

#include <opendspx/audioclip.h>

#include <dspxmodel/BusControl.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class AudioClipPrivate {
        Q_DECLARE_PUBLIC(AudioClip)
    public:
        AudioClip *q_ptr;
        ModelPrivate *pModel;
        QString path;
    };

    AudioClip::AudioClip(Handle handle, Model *model) : Clip(Audio, handle, model), d_ptr(new AudioClipPrivate) {
        Q_D(AudioClip);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_AudioClip);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    AudioClip::~AudioClip() = default;

    QString AudioClip::path() const {
        Q_D(const AudioClip);
        return d->path;
    }

    void AudioClip::setPath(const QString &path) {
        Q_D(AudioClip);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Path, path);
    }

    QDspx::AudioClip AudioClip::toQDspx() const {
        return {
            name(),
            control()->toQDspx(),
            time()->toQDspx(),
            workspace()->toQDspx(),
            path()
        };
    }

    void AudioClip::fromQDspx(const QDspx::AudioClip &clip) {
        setName(clip.name);
        control()->fromQDspx(clip.control);
        time()->fromQDspx(clip.time);
        workspace()->fromQDspx(clip.workspace);
        setPath(clip.path);
    }

    void AudioClip::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(AudioClip);
        switch (property) {
            case ModelStrategy::P_Path: {
                d->path = value.toString();
                Q_EMIT pathChanged(d->path);
                break;
            }
            default:
                Clip::handleSetEntityProperty(property, value);
        }
    }

}
