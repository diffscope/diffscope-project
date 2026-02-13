#include "EditActionsAddOn.h"

#include <QtGlobal>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSelectionModel.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/TrackSelectionModel.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    using Direction = EditActionsAddOn::ShiftCursorDirection;

    static bool isBackward(Direction direction) {
        return direction == EditActionsAddOn::ShiftCursorDirection_Left
            || direction == EditActionsAddOn::ShiftCursorDirection_Up;
    }

    template<typename Sequence, typename Item>
    static Item *advanceInSequence(Sequence *sequence, Item *current, bool backward) {
        return current ? (backward ? sequence->previousItem(current) : sequence->nextItem(current))
                       : sequence->firstItem();
    }

    static dspx::Label *resolveLabelTarget(dspx::Model *model, dspx::Label *current, bool backward) {
        return advanceInSequence(model->timeline()->labels(), current, backward);
    }

    static dspx::Tempo *resolveTempoTarget(dspx::Model *model, dspx::Tempo *current, bool backward) {
        return advanceInSequence(model->timeline()->tempos(), current, backward);
    }

    static dspx::Note *resolveNoteTarget(dspx::SelectionModel *selectionModel, dspx::Note *current, bool backward) {
        auto *noteSequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
        return advanceInSequence(noteSequence, current, backward);
    }

    static dspx::Track *resolveTrackTarget(dspx::Model *model, dspx::Track *current, bool backward) {
        const auto items = model->tracks()->items();
        if (items.isEmpty()) {
            return nullptr;
        }
        if (!current) {
            return items.front();
        }

        const auto index = items.indexOf(current);
        Q_ASSERT(index >= 0);

        if (backward && index > 0) {
            return items.at(index - 1);
        }
        if (!backward && index + 1 < items.size()) {
            return items.at(index + 1);
        }
        return items.at(index);
    }

    EditActionsAddOn::EditActionsAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    EditActionsAddOn::~EditActionsAddOn() = default;

    void EditActionsAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditActionsAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void EditActionsAddOn::extensionsInitialized() {
    }

    bool EditActionsAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    void EditActionsAddOn::shiftCursor(ShiftCursorDirection direction) {
        auto *document = windowHandle()->cast<ProjectWindowInterface>()->projectDocumentContext()->document();
        auto *selectionModel = document->selectionModel();
        auto *model = document->model();

        const auto selectionType = selectionModel->selectionType();
        auto *currentItem = selectionModel->currentItem();
        const bool backward = isBackward(direction);

        QObject *target = nullptr;
        switch (selectionType) {
            case dspx::SelectionModel::ST_None:
                return;
            case dspx::SelectionModel::ST_AnchorNode:
                // TODO support anchor node cursor shifting
                break;
            case dspx::SelectionModel::ST_Clip:
                // TODO support clip cursor shifting
                break;
            case dspx::SelectionModel::ST_Label:
                target = resolveLabelTarget(model, static_cast<dspx::Label *>(currentItem), backward);
                break;
            case dspx::SelectionModel::ST_Note:
                target = resolveNoteTarget(selectionModel, static_cast<dspx::Note *>(currentItem), backward);
                break;
            case dspx::SelectionModel::ST_Tempo:
                target = resolveTempoTarget(model, static_cast<dspx::Tempo *>(currentItem), backward);
                break;
            case dspx::SelectionModel::ST_Track:
                target = resolveTrackTarget(model, static_cast<dspx::Track *>(currentItem), backward);
                break;
        }

        if (target) {
            selectionModel->select(target, dspx::SelectionModel::SetCurrentItem);
        }
    }
    void EditActionsAddOn::selectCurrent() {
        auto selectionModel = windowHandle()->cast<ProjectWindowInterface>()->projectDocumentContext()->document()->selectionModel();
        if (!selectionModel->currentItem())
            return;
        selectionModel->select(selectionModel->currentItem(), dspx::SelectionModel::ClearPreviousSelection | dspx::SelectionModel::Select);
    }
    void EditActionsAddOn::multipleSelectCurrent() {
        auto selectionModel = windowHandle()->cast<ProjectWindowInterface>()->projectDocumentContext()->document()->selectionModel();
        if (!selectionModel->currentItem())
            return;
        selectionModel->select(selectionModel->currentItem(), dspx::SelectionModel::Toggle);
    }

}
