#include "AudioClip.h"

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/BusControl.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/Workspace.h>

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
        QDspx::AudioClip clip;
        clip.control = control()->toQDspx();
        clip.name = name();
        clip.path = path();
        clip.time = time()->toQDspx();
        clip.type = QDspx::Clip::Audio;
        clip.workspace = workspace()->toQDspx();
        return clip;
    }

    void AudioClip::fromQDspx(const QDspx::AudioClip &clip) {
        control()->fromQDspx(clip.control);
        setName(clip.name);
        setPath(clip.path);
        time()->fromQDspx(clip.time);
        workspace()->fromQDspx(clip.workspace);
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