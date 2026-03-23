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

    opendspx::AudioClip AudioClip::toOpenDspx() const {
        return {
            name().toStdString(),
            control()->toOpenDspx(),
            time()->toOpenDspx(),
            workspace()->toOpenDspx(),
            path().toStdString()
        };
    }

    void AudioClip::fromOpenDspx(const opendspx::AudioClip &clip) {
        setName(QString::fromStdString(clip.name));
        control()->fromOpenDspx(clip.control);
        time()->fromOpenDspx(clip.time);
        workspace()->fromOpenDspx(clip.workspace);
        setPath(QString::fromStdString(clip.path));
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
