#include "SelectionModel.h"
#include "SelectionModel_p.h"

#include <dspxmodel/Model.h>
#include <dspxmodel/AnchorNode.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/private/AnchorNodeSelectionModel_p.h>
#include <dspxmodel/private/ClipSelectionModel_p.h>
#include <dspxmodel/private/LabelSelectionModel_p.h>
#include <dspxmodel/private/NoteSelectionModel_p.h>
#include <dspxmodel/private/TempoSelectionModel_p.h>
#include <dspxmodel/private/TrackSelectionModel_p.h>

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

    SelectionModel::SelectionType SelectionModel::selectionTypeFromItem(QObject *item) {
        if (qobject_cast<AnchorNode *>(item)) {
            return ST_AnchorNode;
        }
        if (qobject_cast<Clip *>(item)) {
            return ST_Clip;
        }
        if (qobject_cast<Label *>(item)) {
            return ST_Label;
        }
        if (qobject_cast<Note *>(item)) {
            return ST_Note;
        }
        if (qobject_cast<Tempo *>(item)) {
            return ST_Tempo;
        }
        if (qobject_cast<Track *>(item)) {
            return ST_Track;
        }
        return ST_None;
    }

    void SelectionModel::select(QObject *item, SelectionCommand command) {
        Q_D(SelectionModel);
        auto targetSelectionType = selectionTypeFromItem(item);
        if (targetSelectionType != d->selectionType) {
            switch (selectionType) {
                case ST_AnchorNode:
                    // d->anchorNodeSelectionModel->d_func()->select(nullptr, ClearPreviousSelection);
                    break;
                case ST_Clip:
                    d->clipSelectionModel->d_func()->select(nullptr, ClearPreviousSelection);
                    break;
                case ST_Label:
                    d->labelSelectionModel->d_func()->select(nullptr, ClearPreviousSelection);
                    break;
                case ST_Note:
                    // d->noteSelectionModel->d_func()->select(nullptr, ClearPreviousSelection);
                    break;
                case ST_Tempo:
                    d->tempoSelectionModel->d_func()->select(nullptr, ClearPreviousSelection);
                    break;
                case ST_Track:
                    // d->trackSelectionModel->d_func()->select(nullptr, ClearPreviousSelection);
                    break;
                default:
                    break;
            }
        }
        d->selectionType = targetSelectionType;
        switch (targetSelectionType) {
            case ST_AnchorNode:
                // d->anchorNodeSelectionModel->d_func()->select(item, command);
                break;
            case ST_Clip:
                d->clipSelectionModel->d_func()->select(static_cast<Clip *>(item), command);
                break;
            case ST_Label:
                d->labelSelectionModel->d_func()->select(static_cast<Label *>(item), command);
                break;
            case ST_Note:
                // d->noteSelectionModel->d_func()->select(item, command);
                break;
            case ST_Tempo:
                d->tempoSelectionModel->d_func()->select(static_cast<Tempo *>(item), command);
                break;
            case ST_Track:
                // d->trackSelectionModel->d_func()->select(item, command);
                break;
            default:
                break;
        }
    }

}

#include "moc_SelectionModel.cpp"
