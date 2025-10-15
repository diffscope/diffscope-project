#include "Track.h"

#include <QVariant>
#include <QColor>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/TrackControl.h>
#include <dspxmodel/Workspace.h>

namespace dspx {

    class TrackPrivate {
        Q_DECLARE_PUBLIC(Track)
    public:
        Track *q_ptr;
        ModelPrivate *pModel;
        QColor color;
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
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->color = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Color).value<QColor>();
        d->name = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Name).toString();
        d->control = d->pModel->createObject<TrackControl>(handle);
        d->workspace = d->pModel->createObject<Workspace>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Workspace));
    }

    Track::~Track() = default;

    QColor Track::color() const {
        Q_D(const Track);
        return d->color;
    }

    void Track::setColor(const QColor &color) {
        Q_D(Track);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Color, color);
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
            name(),
            control()->toQDspx(),
            {},
            {},
            {},
            {},
            workspace()->toQDspx()
        };
    }

    void Track::fromQDspx(const QDspx::Track &track) {
        setName(track.name);
        control()->fromQDspx(track.control);
        workspace()->fromQDspx(track.workspace);
    }

    void Track::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Track);
        switch (property) {
            case ModelStrategy::P_Color: {
                d->color = value.value<QColor>();
                Q_EMIT colorChanged(d->color);
                break;
            }
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