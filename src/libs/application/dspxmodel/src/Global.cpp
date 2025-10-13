#include "Global.h"

#include <QVariant>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class GlobalPrivate {
        Q_DECLARE_PUBLIC(Global)
    public:
        Global *q_ptr;
        ModelPrivate *pModel;
    };

    Global::Global(Model *model) : QObject(model), d_ptr(new GlobalPrivate) {
        Q_D(Global);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }
    Global::~Global() = default;

    QString Global::name() const {
        Q_D(const Global);
        return d->pModel->name;
    }
    void Global::setName(const QString &name) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->pModel->hGlobal, ModelStrategy::P_Name, name);
    }
    QString Global::author() const {
        Q_D(const Global);
        return d->pModel->author;
    }
    void Global::setAuthor(const QString &author) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->pModel->hGlobal, ModelStrategy::P_Author, author);
    }
    int Global::centShift() const {
        Q_D(const Global);
        return d->pModel->centShift;
    }
    void Global::setCentShift(int centShift) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->pModel->hGlobal, ModelStrategy::P_CentShift, centShift);
    }
    QString Global::editorId() const {
        Q_D(const Global);
        return d->pModel->editorId;
    }
    void Global::setEditorId(const QString &editorId) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->pModel->hGlobal, ModelStrategy::P_EditorId, editorId);
    }
    QString Global::editorName() const {
        Q_D(const Global);
        return d->pModel->editorName;
    }
    void Global::setEditorName(const QString &editorName) {
        Q_D(Global);
        d->pModel->strategy->setEntityProperty(d->pModel->hGlobal, ModelStrategy::P_EditorName, editorName);
    }

}