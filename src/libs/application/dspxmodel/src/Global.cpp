#include "Global.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class GlobalPrivate {
        Q_DECLARE_PUBLIC(Global)
    public:
        Global *q_ptr;
        ModelPrivate *pModel;
        Handle handle;

        int centShift() const;
        void setCentShiftUnchecked(int centShift);
        void setCentShift(int centShift);

    };

    int GlobalPrivate::centShift() const {
        return pModel->centShift;
    }

    void GlobalPrivate::setCentShiftUnchecked(int centShift) {
        Q_Q(Global);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_CentShift, centShift);
    }

    void GlobalPrivate::setCentShift(int centShift) {
        Q_Q(Global);
        if (auto engine = qjsEngine(q); engine && (centShift < -50 || centShift > 50)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Cent shift must be in range [-50, 50]"));
            return;
        }
        setCentShiftUnchecked(centShift);
    }

    Global::Global(Model *model) : QObject(model), d_ptr(new GlobalPrivate) {
        Q_D(Global);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->handle = model->handle();
    }
    Global::~Global() = default;

    QString Global::name() const {
        Q_D(const Global);
        return d->pModel->name;
    }
    void Global::setName(const QString &name) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_Name, name);
    }
    QString Global::author() const {
        Q_D(const Global);
        return d->pModel->author;
    }
    void Global::setAuthor(const QString &author) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_Author, author);
    }
    int Global::centShift() const {
        Q_D(const Global);
        return d->centShift();
    }
    void Global::setCentShift(int centShift) {
        Q_D(Global);
        Q_ASSERT(centShift >= -50 && centShift <= 50);
        d->setCentShiftUnchecked(centShift);
    }
    QString Global::editorId() const {
        Q_D(const Global);
        return d->pModel->editorId;
    }
    void Global::setEditorId(const QString &editorId) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_EditorId, editorId);
    }
    QString Global::editorName() const {
        Q_D(const Global);
        return d->pModel->editorName;
    }
    void Global::setEditorName(const QString &editorName) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_EditorName, editorName);
    }

    QDspx::Global Global::toQDspx() const {
        return {
            name(),
            author(),
            centShift(),
            // TODO editorId editorName
        };
    }

    void Global::fromQDspx(const QDspx::Global &global) {
        setName(global.name);
        setAuthor(global.author);
        setCentShift(global.centShift);
        // TODO editorId editorName
    }

}

#include "moc_Global.cpp"