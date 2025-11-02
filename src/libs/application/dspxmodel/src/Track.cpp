#include "Track.h"

#include <QVariant>
#include <QColor>

#include <opendspx/track.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/TrackControl.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/ClipSequence.h>

namespace dspx {

    class TrackPrivate {
        Q_DECLARE_PUBLIC(Track)
    public:
        Track *q_ptr;
        ModelPrivate *pModel;
        ClipSequence *clips;
        QString name;
        TrackControl *control;
        Workspace *workspace;
    };

    static QString colorToHex(const QColor &color) {
        return color.isValid() ? color.name(QColor::HexRgb) : QString();
    }

    static QColor hexToColor(const QString &hex) {
        return QColor::fromString(hex);
    }

    Track::Track(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new TrackPrivate) {
        Q_D(Track);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Track);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->name = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Name).toString();
        d->control = d->pModel->createObject<TrackControl>(handle);
        d->workspace = d->pModel->createObject<Workspace>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Workspace));
        d->clips = d->pModel->createObject<ClipSequence>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Children));
        d->clips->setTrack(this);
    }

    Track::~Track() = default;

    ClipSequence * Track::clips() const {
        Q_D(const Track);
        return d->clips;
    }

    TrackControl *Track::control() const {
        Q_D(const Track);
        return d->control;
    }

    QString Track::name() const {
        Q_D(const Track);
        return d->name;
    }

    void Track::setName(const QString &name) {
        Q_D(Track);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Name, name);
    }

    Workspace *Track::workspace() const {
        Q_D(const Track);
        return d->workspace;
    }

    QDspx::Track Track::toQDspx() const {
        return {
            .name = name(),
            .control = control()->toQDspx(),
            .clips = clips()->toQDspx(),
            .workspace = workspace()->toQDspx(),
        };
    }

    void Track::fromQDspx(const QDspx::Track &track) {
        setName(track.name);
        control()->fromQDspx(track.control);
        clips()->fromQDspx(track.clips);
        workspace()->fromQDspx(track.workspace);
    }

    void Track::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Track);
        switch (property) {
            case ModelStrategy::P_Name: {
                d->name = value.toString();
                Q_EMIT nameChanged(d->name);
                break;
            }
            case ModelStrategy::P_ControlGain:
            case ModelStrategy::P_ControlPan:
            case ModelStrategy::P_ControlMute:
            case ModelStrategy::P_ControlSolo: {
                ModelPrivate::proxySetEntityPropertyNotify(d->control, property, value);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Track.cpp"