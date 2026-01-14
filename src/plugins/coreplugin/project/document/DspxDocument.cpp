#include "DspxDocument.h"
#include "DspxDocument_p.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

#include <QLoggingCategory>
#include <QUndoStack>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/DspxClipboard.h>

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/TempoSelectionModel.h>
#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/TrackSelectionModel.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/UndoableModelStrategy.h>

#include <transactional/TransactionController.h>
#include <transactional/TransactionalStrategy.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcDspxDocument, "diffscope.core.dspxdocument")

    static const char *clipboardTypeName(DspxClipboardData::Type type) {
        switch (type) {
            case DspxClipboardData::Tempo:
                return "Tempo";
            case DspxClipboardData::Label:
                return "Label";
            case DspxClipboardData::Track:
                return "Track";
            case DspxClipboardData::Clip:
                return "Clip";
            case DspxClipboardData::Note:
                return "Note";
        }
        return "Unknown";
    }

    static const char *selectionTypeName(dspx::SelectionModel::SelectionType type) {
        switch (type) {
            case dspx::SelectionModel::ST_None:
                return "None";
            case dspx::SelectionModel::ST_AnchorNode:
                return "AnchorNode";
            case dspx::SelectionModel::ST_Clip:
                return "Clip";
            case dspx::SelectionModel::ST_Label:
                return "Label";
            case dspx::SelectionModel::ST_Note:
                return "Note";
            case dspx::SelectionModel::ST_Tempo:
                return "Tempo";
            case dspx::SelectionModel::ST_Track:
                return "Track";
        }
        return "Unknown";
    }

    class TransactionalModelStrategy : public TransactionalStrategy {
    public:
        explicit TransactionalModelStrategy(dspx::UndoableModelStrategy *undoableModelStrategy, QObject *parent = nullptr)
            : TransactionalStrategy(parent), m_strategy(undoableModelStrategy) {
        }

        void beginTransaction() override {
            m_strategy->undoStack()->beginMacro("");
        }
        void abortTransaction() override {
            m_strategy->undoStack()->endMacro();
            m_strategy->undoStack()->undo();
        }
        void commitTransaction() override {
            m_strategy->undoStack()->endMacro();
        }
        void moveCurrentStepBy(int count) override {
            m_strategy->undoStack()->setIndex(m_strategy->undoStack()->index() + count);
        }

    private:
        dspx::UndoableModelStrategy *m_strategy;

    };

    template<typename Signal>
    bool DspxDocumentPrivate::emitOnChange(bool value, bool &cache, Signal signal) {
        if (cache == value)
            return false;
        cache = value;
        Q_EMIT (q_ptr->*signal)();
        return true;
    }

    void DspxDocumentPrivate::initConnections() {
        updateAnyItemsSelected();
        updateEditScopeFocused();
        updatePasteAvailable();

        QObject::connect(selectionModel, &dspx::SelectionModel::selectedCountChanged, q_ptr, [this] {
            updateAnyItemsSelected();
        });
        QObject::connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, q_ptr, [this] {
            updateEditScopeFocused();
            updatePasteAvailable();
        });

        if (auto *clipboard = DspxClipboard::instance()) {
            QObject::connect(clipboard, &DspxClipboard::changed, q_ptr, [this] {
                updatePasteAvailable();
            });
        }
    }

    bool DspxDocumentPrivate::updateAnyItemsSelected() {
        const bool value = selectionModel && selectionModel->selectedCount() > 0;
        return emitOnChange(value, anyItemsSelected, &DspxDocument::anyItemsSelectedChanged);
    }

    bool DspxDocumentPrivate::updateEditScopeFocused() {
        const bool value = selectionModel && selectionModel->selectionType() != dspx::SelectionModel::ST_None;
        return emitOnChange(value, editScopeFocused, &DspxDocument::editScopeFocusedChanged);
    }

    std::optional<DspxClipboardData::Type> DspxDocumentPrivate::currentClipboardType() const {
        if (!selectionModel)
            return std::nullopt;

        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Tempo:
                return DspxClipboardData::Tempo;
            case dspx::SelectionModel::ST_Label:
                return DspxClipboardData::Label;
            case dspx::SelectionModel::ST_Track:
                return DspxClipboardData::Track;
            case dspx::SelectionModel::ST_Clip:
                return DspxClipboardData::Clip;
            case dspx::SelectionModel::ST_Note:
                return DspxClipboardData::Note;
            case dspx::SelectionModel::ST_AnchorNode:
                // TODO support anchor node clipboard mapping
                return std::nullopt;
            case dspx::SelectionModel::ST_None:
            default:
                return std::nullopt;
        }
    }

    bool DspxDocumentPrivate::updatePasteAvailable() {
        const auto type = currentClipboardType();
        bool value = false;
        if (type.has_value()) {
            if (auto *clipboard = DspxClipboard::instance()) {
                value = clipboard->availablePasteTypes().contains(type.value());
            }
        }
        return emitOnChange(value, pasteAvailable, &DspxDocument::pasteAvailableChanged);
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildClipboardData(int playheadPosition) const {
        if (!selectionModel)
            return std::nullopt;

        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Tempo:
                return buildTempoClipboardData(playheadPosition);
            case dspx::SelectionModel::ST_Label:
                return buildLabelClipboardData(playheadPosition);
            case dspx::SelectionModel::ST_Track:
                return buildTrackClipboardData();
            case dspx::SelectionModel::ST_Clip:
            case dspx::SelectionModel::ST_Note:
            case dspx::SelectionModel::ST_AnchorNode:
                // TODO support clipboard build for additional selection types
                return std::nullopt;
            case dspx::SelectionModel::ST_None:
            default:
                return std::nullopt;
        }
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildTempoClipboardData(int playheadPosition) const {
        if (!model || !selectionModel)
            return std::nullopt;

        const auto selectedItems = selectionModel->tempoSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;

        QList<QDspx::Tempo> tempos;
        tempos.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            tempos.append(item->toQDspx());
        }
        if (tempos.isEmpty())
            return std::nullopt;

        std::sort(tempos.begin(), tempos.end(), [](const QDspx::Tempo &lhs, const QDspx::Tempo &rhs) {
            return lhs.pos < rhs.pos;
        });

        const int absolute = tempos.first().pos;
        for (auto &tempo : tempos)
            tempo.pos -= absolute;

        DspxClipboardData data;
        data.setTempos(tempos);
        data.setAbsolute(absolute);
        data.setPlayhead(playheadPosition - absolute);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildLabelClipboardData(int playheadPosition) const {
        if (!model || !selectionModel)
            return std::nullopt;

        const auto selectedItems = selectionModel->labelSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;

        QList<QDspx::Label> labels;
        labels.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            labels.append(item->toQDspx());
        }
        if (labels.isEmpty())
            return std::nullopt;

        std::sort(labels.begin(), labels.end(), [](const QDspx::Label &lhs, const QDspx::Label &rhs) {
            return lhs.pos < rhs.pos;
        });

        const int absolute = labels.first().pos;
        for (auto &label : labels)
            label.pos -= absolute;

        DspxClipboardData data;
        data.setLabels(labels);
        data.setAbsolute(absolute);
        data.setPlayhead(playheadPosition - absolute);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildTrackClipboardData() const {
        if (!model || !selectionModel)
            return std::nullopt;

        auto selectedItems = selectionModel->trackSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;

        const auto orderedTracks = model->tracks()->items();
        std::sort(selectedItems.begin(), selectedItems.end(), [&orderedTracks](dspx::Track *lhs, dspx::Track *rhs) {
            return orderedTracks.indexOf(lhs) < orderedTracks.indexOf(rhs);
        });

        QList<QDspx::Track> tracks;
        tracks.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            tracks.append(item->toQDspx());
        }

        if (tracks.isEmpty())
            return std::nullopt;

        DspxClipboardData data;
        data.setTracks(tracks);
        return data;
    }

    bool DspxDocumentPrivate::pasteClipboardData(const DspxClipboardData &data, int playheadPosition) {
        switch (data.type()) {
            case DspxClipboardData::Tempo:
                return pasteTempos(data.tempos(), data, playheadPosition);
            case DspxClipboardData::Label:
                return pasteLabels(data.labels(), data, playheadPosition);
            case DspxClipboardData::Track:
                return pasteTracks(data.tracks());
            case DspxClipboardData::Clip:
            case DspxClipboardData::Note:
                // TODO paste support for clips and notes
                return false;
        }
        return false;
    }

    bool DspxDocumentPrivate::pasteTempos(const QList<QDspx::Tempo> &tempos, const DspxClipboardData &data, int playheadPosition) {
        if (!model || !model->timeline() || tempos.isEmpty())
            return false;

        enum class PasteMode {
            CurrentPosition,
            RelativeToCopiedPlayhead,
            OriginalPosition,
        };

        // TODO allow selecting paste mode from user settings
        const auto pasteMode = PasteMode::CurrentPosition;

        const auto baseOffset = [pasteMode, &data, playheadPosition] {
            switch (pasteMode) {
                case PasteMode::CurrentPosition:
                    return playheadPosition;
                case PasteMode::RelativeToCopiedPlayhead:
                    return playheadPosition - data.playhead();
                case PasteMode::OriginalPosition:
                    return data.absolute();
            }
            return playheadPosition;
        }();

        QList<QDspx::Tempo> adjusted = tempos;
        int minPos = adjusted.isEmpty() ? 0 : std::numeric_limits<int>::max();
        for (auto &tempo : adjusted) {
            tempo.pos += baseOffset;
            minPos = std::min(minPos, tempo.pos);
        }
        if (minPos < 0) {
            const int shift = -minPos;
            for (auto &tempo : adjusted)
                tempo.pos += shift;
        }

        std::sort(adjusted.begin(), adjusted.end(), [](const QDspx::Tempo &lhs, const QDspx::Tempo &rhs) {
            return lhs.pos < rhs.pos;
        });

        bool inserted = false;
        int removedOverlaps = 0;
        auto tempoSequence = model->timeline()->tempos();
        for (const auto &tempoData : adjusted) {
            auto *tempo = model->createTempo();
            tempo->fromQDspx(tempoData);
            if (!tempoSequence->insertItem(tempo)) {
                model->destroyItem(tempo);
                continue;
            }

            inserted = true;
            const auto overlappingItems = tempoSequence->slice(tempoData.pos, 1);
            for (auto *overlappingItem : overlappingItems) {
                if (overlappingItem == tempo)
                    continue;
                tempoSequence->removeItem(overlappingItem);
                model->destroyItem(overlappingItem);
                ++removedOverlaps;
            }
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted tempos" << adjusted.size() << "removed overlaps" << removedOverlaps;
        else
            qCDebug(lcDspxDocument) << "No tempo pasted";

        return inserted;
    }

    bool DspxDocumentPrivate::pasteLabels(const QList<QDspx::Label> &labels, const DspxClipboardData &data, int playheadPosition) {
        if (!model || !model->timeline() || labels.isEmpty())
            return false;

        enum class PasteMode {
            CurrentPosition,
            RelativeToCopiedPlayhead,
            OriginalPosition,
        };

        // TODO allow selecting paste mode from user settings
        const auto pasteMode = PasteMode::CurrentPosition;

        const auto baseOffset = [pasteMode, &data, playheadPosition] {
            switch (pasteMode) {
                case PasteMode::CurrentPosition:
                    return playheadPosition;
                case PasteMode::RelativeToCopiedPlayhead:
                    return playheadPosition - data.playhead();
                case PasteMode::OriginalPosition:
                    return data.absolute();
            }
            return playheadPosition;
        }();

        QList<QDspx::Label> adjusted = labels;
        int minPos = adjusted.isEmpty() ? 0 : std::numeric_limits<int>::max();
        for (auto &label : adjusted) {
            label.pos += baseOffset;
            minPos = std::min(minPos, label.pos);
        }
        if (minPos < 0) {
            const int shift = -minPos;
            for (auto &label : adjusted)
                label.pos += shift;
        }

        std::sort(adjusted.begin(), adjusted.end(), [](const QDspx::Label &lhs, const QDspx::Label &rhs) {
            return lhs.pos < rhs.pos;
        });

        bool inserted = false;
        auto labelSequence = model->timeline()->labels();
        for (const auto &labelData : adjusted) {
            auto *label = model->createLabel();
            label->fromQDspx(labelData);
            if (!labelSequence->insertItem(label)) {
                model->destroyItem(label);
                continue;
            }
            inserted = true;
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted labels" << adjusted.size();
        else
            qCDebug(lcDspxDocument) << "No label pasted";

        return inserted;
    }

    bool DspxDocumentPrivate::pasteTracks(const QList<QDspx::Track> &tracks) {
        if (!model || tracks.isEmpty())
            return false;

        bool inserted = false;
        auto *trackList = model->tracks();
        auto insertionIndex = trackList->size();
        do {
            auto currentTrack = qobject_cast<dspx::Track *>(selectionModel->currentItem());
            if (!currentTrack) {
                break;
            }
            auto index = trackList->items().indexOf(currentTrack);
            if (index == -1) {
                break;
            }
            insertionIndex = index;
        } while (false);
        for (const auto &trackData : tracks) {
            auto *track = model->createTrack();
            track->fromQDspx(trackData);
            if (!trackList->insertItem(insertionIndex, track)) {
                model->destroyItem(track);
                continue;
            }
            insertionIndex++;
            inserted = true;
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted tracks" << tracks.size();
        else
            qCDebug(lcDspxDocument) << "No track pasted";

        return inserted;
    }

    bool DspxDocumentPrivate::deleteSelection() {
        if (!selectionModel || selectionModel->selectedCount() <= 0)
            return false;

        bool removed = false;
        int removedCount = 0;
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Tempo: {
                auto *tempoSequence = model->timeline()->tempos();
                for (auto *item : selectionModel->tempoSelectionModel()->selectedItems()) {
                    if (item->pos() == 0) {
                        const auto overlappingItems = tempoSequence->slice(0, 1);
                        if (overlappingItems.size() == 1) {
                            continue;
                        }
                    }
                    if (tempoSequence->removeItem(item)) {
                        model->destroyItem(item);
                        removed = true;
                        ++removedCount;
                    }
                }
                break;
            }
            case dspx::SelectionModel::ST_Label: {
                auto *labelSequence = model->timeline()->labels();
                for (auto *item : selectionModel->labelSelectionModel()->selectedItems()) {
                    if (labelSequence->removeItem(item)) {
                        model->destroyItem(item);
                        removed = true;
                        ++removedCount;
                    }
                }
                break;
            }
            case dspx::SelectionModel::ST_Track: {
                auto *trackList = model->tracks();
                const auto allTracks = trackList->items();
                QList<int> indexes;
                for (auto *item : selectionModel->trackSelectionModel()->selectedItems()) {
                    const int index = allTracks.indexOf(item);
                    if (index >= 0)
                        indexes.append(index);
                }

                std::sort(indexes.begin(), indexes.end(), [](int lhs, int rhs) {
                    return lhs > rhs;
                });

                for (const int index : std::as_const(indexes)) {
                    auto *removedTrack = trackList->removeItem(index);
                    if (removedTrack) {
                        model->destroyItem(removedTrack);
                        removed = true;
                        ++removedCount;
                    }
                }
                break;
            }
            case dspx::SelectionModel::ST_Clip:
            case dspx::SelectionModel::ST_Note:
            case dspx::SelectionModel::ST_AnchorNode:
                // TODO delete support for additional selection types
                break;
            case dspx::SelectionModel::ST_None:
            default:
                break;
        }

        if (removed)
            qCInfo(lcDspxDocument) << "Deleted selection" << selectionTypeName(selectionModel->selectionType()) << "count" << removedCount;
        else
            qCDebug(lcDspxDocument) << "Delete selection produced no changes" << selectionTypeName(selectionModel->selectionType());

        return removed;
    }

    DspxDocument::DspxDocument(QObject *parent) : QObject(parent), d_ptr(new DspxDocumentPrivate) {
        Q_D(DspxDocument);
        d->q_ptr = this;
        auto modelStrategy = new dspx::UndoableModelStrategy; // TODO use substate in future
        d->model = new dspx::Model(modelStrategy, this);
        modelStrategy->setParent(d->model);
        d->selectionModel = new dspx::SelectionModel(d->model, this);
        auto transactionalStrategy = new TransactionalModelStrategy(modelStrategy);
        d->transactionController = new TransactionController(transactionalStrategy, this);
        transactionalStrategy->setParent(d->transactionController);

        d->initConnections();
    }

    DspxDocument::~DspxDocument() = default;

    dspx::Model *DspxDocument::model() const {
        Q_D(const DspxDocument);
        return d->model;
    }

    dspx::SelectionModel *DspxDocument::selectionModel() const {
        Q_D(const DspxDocument);
        return d->selectionModel;
    }

    TransactionController *DspxDocument::transactionController() const {
        Q_D(const DspxDocument);
        return d->transactionController;
    }

    bool DspxDocument::anyItemsSelected() {
        Q_D(DspxDocument);
        return d->anyItemsSelected;
    }

    bool DspxDocument::isEditScopeFocused() {
        Q_D(DspxDocument);
        return d->editScopeFocused;
    }

    bool DspxDocument::pasteAvailable() {
        Q_D(DspxDocument);
        return d->pasteAvailable;
    }

    void DspxDocument::cutSelection(int playheadPosition) {
        if (!anyItemsSelected())
            return;
        copySelection(playheadPosition);
        deleteSelection();
    }

    void DspxDocument::copySelection(int playheadPosition) {
        Q_D(DspxDocument);
        if (!anyItemsSelected())
            return;
        qCInfo(lcDspxDocument) << "Copy selection";
        const auto clipboardData = d->buildClipboardData(playheadPosition);
        if (!clipboardData.has_value())
            return;

        if (auto *clipboard = DspxClipboard::instance())
            clipboard->copy({clipboardData.value()}, nullptr);

        auto copiedType = clipboardData->type();
        int copiedCount = 0;
        switch (copiedType) {
            case DspxClipboardData::Tempo:
                copiedCount = clipboardData->tempos().size();
                break;
            case DspxClipboardData::Label:
                copiedCount = clipboardData->labels().size();
                break;
            case DspxClipboardData::Track:
                copiedCount = clipboardData->tracks().size();
                break;
            case DspxClipboardData::Clip:
            case DspxClipboardData::Note:
                copiedCount = 0;
                break;
        }
        qCInfo(lcDspxDocument) << "Copied selection" << clipboardTypeName(copiedType) << "count" << copiedCount;

        d->updatePasteAvailable();
    }

    void DspxDocument::paste(int playheadPosition) {
        Q_D(DspxDocument);
        if (!pasteAvailable())
            return;
        const auto type = d->currentClipboardType();
        auto *clipboard = DspxClipboard::instance();
        if (!clipboard || !type.has_value())
            return;

        bool ok = false;
        const auto data = clipboard->paste(type.value(), &ok);
        if (!ok) {
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), nullptr, tr("Paste failed"), tr("Cannot paste data from the clipboard."));
            return;
        }

        qCInfo(lcDspxDocument) << "Pasting" << clipboardTypeName(data.type()) << "playhead" << playheadPosition;

        const QString transactionName = [this, &data] {
            switch (data.type()) {
                case DspxClipboardData::Tempo:
                    return tr("Pasting tempo");
                case DspxClipboardData::Label:
                    return tr("Pasting label");
                case DspxClipboardData::Track:
                    return tr("Pasting track");
                case DspxClipboardData::Clip:
                    return tr("Pasting clip");
                case DspxClipboardData::Note:
                    return tr("Pasting note");
            }
            return tr("Pasting selection");
        }();

        bool pasted = false;
        transactionController()->beginScopedTransaction(transactionName, [=, &pasted, &data] {
            pasted = d->pasteClipboardData(data, playheadPosition);
            return pasted;
        }, [] {
            qCCritical(lcDspxDocument()) << "Failed to paste in scoped transaction";
        });

        qCInfo(lcDspxDocument) << "Paste result" << clipboardTypeName(data.type()) << (pasted ? "succeeded" : "no changes");
    }

    void DspxDocument::deleteSelection() {
        Q_D(DspxDocument);
        if (!anyItemsSelected())
            return;
        qCInfo(lcDspxDocument) << "Delete selection";
        bool deleted = false;
        transactionController()->beginScopedTransaction(tr("Deleting selection"), [=, &deleted] {
            deleted = d->deleteSelection();
            return deleted;
        }, [] {
            qCCritical(lcDspxDocument()) << "Failed to delete selection in scoped transaction";
        });
    }

}

#include "moc_DspxDocument.cpp"
