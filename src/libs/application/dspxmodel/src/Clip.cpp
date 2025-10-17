#include "Clip.h"

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/BusControl.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/AudioClip.h>
#include <dspxmodel/SingingClip.h>

namespace dspx {

    class ClipPrivate {
        Q_DECLARE_PUBLIC(Clip)
    public:
        Clip *q_ptr;
        ModelPrivate *pModel;
        QString name;
        BusControl *control;
        ClipTime *time;
        Clip::ClipType type;
        Track *track{};
        Workspace *workspace;
    };

    Clip::Clip(ClipType type, Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new ClipPrivate) {
        Q_D(Clip);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->name = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Name).toString();
        d->control = d->pModel->createObject<BusControl>(handle);
        d->time = d->pModel->createObject<ClipTime>(handle);
        d->type = type;
        d->workspace = d->pModel->createObject<Workspace>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Workspace));
    }

    Clip::~Clip() = default;

    QString Clip::name() const {
        Q_D(const Clip);
        return d->name;
    }

    void Clip::setName(const QString &name) {
        Q_D(Clip);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Name, name);
    }

    BusControl *Clip::control() const {
        Q_D(const Clip);
        return d->control;
    }

    ClipTime *Clip::time() const {
        Q_D(const Clip);
        return d->time;
    }

    Clip::ClipType Clip::type() const {
        Q_D(const Clip);
        return d->type;
    }

    Workspace *Clip::workspace() const {
        Q_D(const Clip);
        return d->workspace;
    }

    QDspx::ClipRef Clip::toQDspx() const {
        Q_D(const Clip);
        switch (d->type) {
            case Audio:
                return QSharedPointer<QDspx::AudioClip>::create(static_cast<const AudioClip *>(this)->toQDspx());
            case Singing:
                return QSharedPointer<QDspx::SingingClip>::create(static_cast<const SingingClip *>(this)->toQDspx());
            default:
                Q_UNREACHABLE();
        }
    }

    void Clip::fromQDspx(const QDspx::ClipRef &clip) {
        switch (clip->type) {
            case QDspx::Clip::Audio:
                static_cast<AudioClip *>(this)->fromQDspx(*clip.staticCast<QDspx::AudioClip>());
                break;
            case QDspx::Clip::Singing:
                static_cast<SingingClip *>(this)->fromQDspx(*clip.staticCast<QDspx::SingingClip>());
                break;
            default:
                Q_UNREACHABLE();
        }
    }

    Track *Clip::track() const {
        Q_D(const Clip);
        return d->track;
    }

    void Clip::setTrack(Track *track) {
        Q_D(Clip);
        if (d->track != track) {
            d->track = track;
            Q_EMIT trackChanged();
        }
    }

    void Clip::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Clip);
        switch (property) {
            case ModelStrategy::P_Name: {
                d->name = value.toString();
                Q_EMIT nameChanged(d->name);
                break;
            }
            case ModelStrategy::P_ControlGain:
            case ModelStrategy::P_ControlPan:
            case ModelStrategy::P_ControlMute: {
                ModelPrivate::proxySetEntityPropertyNotify(d->control, property, value);
                break;
            }
            case ModelStrategy::P_Position:
            case ModelStrategy::P_Length:
            case ModelStrategy::P_ClipStart:
            case ModelStrategy::P_ClipLength: {
                ModelPrivate::proxySetEntityPropertyNotify(d->time, property, value);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }
}

#include "moc_Clip.cpp"