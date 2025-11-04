#include "DspxDocument.h"
#include "DspxDocument_p.h"

#include <dspxmodel/Model.h>
#include <dspxmodel/BasicModelStrategy.h>

namespace Core {

    DspxDocument::DspxDocument(QObject *parent) : QObject(parent), d_ptr(new DspxDocumentPrivate) {
        Q_D(DspxDocument);
        d->q_ptr = this;
        auto strategy = new dspx::BasicModelStrategy; // TODO use substate in future
        d->model = new dspx::Model(strategy, this);
        strategy->setParent(d->model);
    }

    DspxDocument::~DspxDocument() = default;

    dspx::Model * DspxDocument::model() const {
        Q_D(const DspxDocument);
        return d->model;
    }

}

#include "moc_DspxDocument.cpp"
