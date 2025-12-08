#include "Track.h"
#include "Track_p.h"

#include <QColor>
#include <QVariant>

#include <opendspx/track.h>

#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/TrackControl.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/ClipSequence_p.h>

namespace dspx {

    void TrackPrivate::setTrackList(Track *item, TrackList *trackList) {
        auto d = item->d_func();
        if (d->trackList != trackList) {
            d->trackList = trackList;
            Q_EMIT item->trackListChanged();
        }
    }

    Track::Track(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new TrackPrivate) {
        Q_D(Track);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Track);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->name = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Name).toString();
        d->colorId = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ColorId).toInt();
        d->control = d->pModel->createObject<TrackControl>(handle);
        d->workspace = d->pModel->createObject<Workspace>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Workspace));
        d->clips = d->pModel->createObject<ClipSequence>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Children));
    }

    Track::~Track() = default;

    ClipSequence *Track::clips() const {
        Q_D(const Track);
        return d->clips;
    }

    int Track::colorId() const {
        Q_D(const Track);
        return d->colorId;
    }

    void Track::setColorId(int colorId) {
        Q_D(Track);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_ColorId, colorId);
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
        QDspx::Track track {
            .name = name(),
            .control = control()->toQDspx(),
            .clips = clips()->toQDspx(),
            .workspace = workspace()->toQDspx(),
        };
        track.workspace["diffscope"]["colorId"] = colorId();
        return track;
    }

    void Track::fromQDspx(const QDspx::Track &track) {
        setName(track.name);
        control()->fromQDspx(track.control);
        clips()->fromQDspx(track.clips);
        workspace()->fromQDspx(track.workspace);
        setColorId(track.workspace["diffscope"]["colorId"].toInt());
    }

    TrackList *Track::trackList() const {
        Q_D(const Track);
        return d->trackList;
    }

    void Track::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Track);
        switch (property) {
            case ModelStrategy::P_Name: {
                d->name = value.toString();
                Q_EMIT nameChanged(d->name);
                break;
            }
            case ModelStrategy::P_ColorId: {
                d->colorId = value.toInt();
                Q_EMIT colorIdChanged(d->colorId);
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
