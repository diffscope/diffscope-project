#include "SelectionModel.h"
#include "SelectionModel_p.h"

#include <dspxmodel/AnchorNodeSelectionModel.h>
#include <dspxmodel/ClipSelectionModel.h>
#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/TempoSelectionModel.h>
#include <dspxmodel/TrackSelectionModel.h>
#include <dspxmodel/Model.h>

namespace dspx {

    SelectionModel::SelectionModel(Model *model, QObject *parent) : QObject(parent), d_ptr(new SelectionModelPrivate) {
        Q_D(SelectionModel);
        d->q_ptr = this;
        d->model = model;
        d->anchorNodeSelectionModel = new AnchorNodeSelectionModel(this);
        d->clipSelectionModel = new ClipSelectionModel(this);
        d->labelSelectionModel = new LabelSelectionModel(this);
        d->noteSelectionModel = new NoteSelectionModel(this);
        d->tempoSelectionModel = new TempoSelectionModel(this);
        d->trackSelectionModel = new TrackSelectionModel(this);
    }

    SelectionModel::~SelectionModel() = default;

    Model *SelectionModel::model() const {
        Q_D(const SelectionModel);
        return d->model;
    }

    SelectionModel::SelectionType SelectionModel::selectionType() const {
        Q_D(const SelectionModel);
        return d->selectionType;
    }

    AnchorNodeSelectionModel *SelectionModel::anchorNodeSelectionModel() const {
        Q_D(const SelectionModel);
        return d->anchorNodeSelectionModel;
    }

    ClipSelectionModel *SelectionModel::clipSelectionModel() const {
        Q_D(const SelectionModel);
        return d->clipSelectionModel;
    }

    LabelSelectionModel *SelectionModel::labelSelectionModel() const {
        Q_D(const SelectionModel);
        return d->labelSelectionModel;
    }

    NoteSelectionModel *SelectionModel::noteSelectionModel() const {
        Q_D(const SelectionModel);
        return d->noteSelectionModel;
    }

    TempoSelectionModel *SelectionModel::tempoSelectionModel() const {
        Q_D(const SelectionModel);
        return d->tempoSelectionModel;
    }

    TrackSelectionModel *SelectionModel::trackSelectionModel() const {
        Q_D(const SelectionModel);
        return d->trackSelectionModel;
    }

    void SelectionModel::select(QObject *item, SelectionCommand command) {
    }

}

#include "moc_SelectionModel.cpp"
