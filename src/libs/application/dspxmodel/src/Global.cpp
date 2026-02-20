#include "Global.h"
#include <QVariant>

#include <opendspx/global.h>

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
    };

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
        return d->pModel->centShift;
    }
    void Global::setCentShift(int centShift) {
        Q_D(Global);
        Q_ASSERT(centShift >= -50 && centShift <= 50);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_CentShift, centShift);
    }
    Global::AccidentalType Global::accidentalType() const {
        Q_D(const Global);
        return static_cast<AccidentalType>(d->pModel->accidentalType);
    }
    void Global::setAccidentalType(AccidentalType accidentalType) {
        Q_D(Global);
        Q_ASSERT(accidentalType == Flat || accidentalType == Sharp);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_AccidentalType, accidentalType);
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
            .author = author(),
            .name = name(),
            .centShift = centShift(),
            .editorId = editorId(),
            .editorName = editorName()
        };
    }

    void Global::fromQDspx(const QDspx::Global &global) {
        setAuthor(global.author);
        setName(global.name);
        setCentShift(global.centShift);
        setEditorId(global.editorId);
        setEditorName(global.editorName);
    }

}

#include "moc_Global.cpp"
