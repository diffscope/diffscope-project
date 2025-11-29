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

    template<class T>
    static void connectSignals(T *itemSelectionModel, SelectionModel *selectionModel) {
        QObject::connect(itemSelectionModel, &T::currentItemChanged, selectionModel, &SelectionModel::currentItemChanged);
        QObject::connect(itemSelectionModel, &T::selectedCountChanged, selectionModel, &SelectionModel::selectedCountChanged);
        QObject::connect(itemSelectionModel, &T::itemSelected, selectionModel, &SelectionModel::itemSelected);
    }

    SelectionModel::SelectionModel(Model *model, QObject *parent) : QObject(parent), d_ptr(new SelectionModelPrivate) {
        Q_D(SelectionModel);
        d->q_ptr = this;
        d->model = model;
        d->anchorNodeSelectionModel = new AnchorNodeSelectionModel(this);
        // connectSignals(d->anchorNodeSelectionModel, this);
        d->clipSelectionModel = new ClipSelectionModel(this);
        connectSignals(d->clipSelectionModel, this);
        d->labelSelectionModel = new LabelSelectionModel(this);
        connectSignals(d->labelSelectionModel, this);
        d->noteSelectionModel = new NoteSelectionModel(this);
        connectSignals(d->noteSelectionModel, this);
        d->tempoSelectionModel = new TempoSelectionModel(this);
        connectSignals(d->tempoSelectionModel, this);
        d->trackSelectionModel = new TrackSelectionModel(this);
        connectSignals(d->trackSelectionModel, this);
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

    QObject *SelectionModel::currentItemSelectionModel() const {
        Q_D(const SelectionModel);
        switch (d->selectionType) {
            case ST_AnchorNode: return d->anchorNodeSelectionModel;
            case ST_Clip: return d->clipSelectionModel;
            case ST_Label: return d->labelSelectionModel;
            case ST_Note: return d->noteSelectionModel;
            case ST_Tempo: return d->tempoSelectionModel;
            case ST_Track: return d->trackSelectionModel;
            default: return nullptr;
        }
    }

    QObject *SelectionModel::currentItem() const {
        Q_D(const SelectionModel);
        switch (d->selectionType) {
            case ST_AnchorNode: return d->anchorNodeSelectionModel->currentItem();
            case ST_Clip: return d->clipSelectionModel->currentItem();
            case ST_Label: return d->labelSelectionModel->currentItem();
            case ST_Note: return d->noteSelectionModel->currentItem();
            case ST_Tempo: return d->tempoSelectionModel->currentItem();
            case ST_Track: return d->trackSelectionModel->currentItem();
            default: return nullptr;
        }
    }

    int SelectionModel::selectedCount() const {
        Q_D(const SelectionModel);
        switch (d->selectionType) {
            case ST_AnchorNode: return d->anchorNodeSelectionModel->selectedCount();
            case ST_Clip: return d->clipSelectionModel->selectedCount();
            case ST_Label: return d->labelSelectionModel->selectedCount();
            case ST_Note: return d->noteSelectionModel->selectedCount();
            case ST_Tempo: return d->tempoSelectionModel->selectedCount();
            case ST_Track: return d->trackSelectionModel->selectedCount();
            default: return 0;
        }
    }

    bool SelectionModel::isItemSelected(QObject *item) const {
        Q_D(const SelectionModel);
        switch (d->selectionType) {
            // case ST_AnchorNode: return d->anchorNodeSelectionModel->isItemSelected(qobject_cast<AnchorNode *>(item));
            case ST_Clip: return d->clipSelectionModel->isItemSelected(qobject_cast<Clip *>(item));
            case ST_Label: return d->labelSelectionModel->isItemSelected(qobject_cast<Label *>(item));
            case ST_Note: return d->noteSelectionModel->isItemSelected(qobject_cast<Note *>(item));
            case ST_Tempo: return d->tempoSelectionModel->isItemSelected(qobject_cast<Tempo *>(item));
            case ST_Track: return d->trackSelectionModel->isItemSelected(qobject_cast<Track *>(item));
            default: return false;
        }
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
            switch (d->selectionType) {
                case ST_AnchorNode:
                    // d->anchorNodeSelectionModel->d_func()->clearAll();
                    break;
                case ST_Clip:
                    d->clipSelectionModel->d_func()->clearAll();
                    break;
                case ST_Label:
                    d->labelSelectionModel->d_func()->clearAll();
                    break;
                case ST_Note:
                    d->noteSelectionModel->d_func()->clearAll();
                    break;
                case ST_Tempo:
                    d->tempoSelectionModel->d_func()->clearAll();
                    break;
                case ST_Track:
                    d->trackSelectionModel->d_func()->clearAll();
                    break;
                default:
                    break;
            }
        }
        d->selectionType = targetSelectionType;
        Q_EMIT selectionTypeChanged();
        Q_EMIT currentItemSelectionModelChanged();
        switch (targetSelectionType) {
            case ST_AnchorNode:
                // d->anchorNodeSelectionModel->d_func()->select(static_cast<AnchorNode *>(item), command);
                break;
            case ST_Clip:
                d->clipSelectionModel->d_func()->select(static_cast<Clip *>(item), command);
                break;
            case ST_Label:
                d->labelSelectionModel->d_func()->select(static_cast<Label *>(item), command);
                break;
            case ST_Note:
                d->noteSelectionModel->d_func()->select(static_cast<Note *>(item), command);
                break;
            case ST_Tempo:
                d->tempoSelectionModel->d_func()->select(static_cast<Tempo *>(item), command);
                break;
            case ST_Track:
                d->trackSelectionModel->d_func()->select(static_cast<Track *>(item), command);
                break;
            default:
                break;
        }
    }

}

#include "moc_SelectionModel.cpp"
