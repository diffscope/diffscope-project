#include "ClipPropertyMapper.h"
#include "ClipPropertyMapper_p.h"

#include <dspxmodel/Clip.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/AnchorNode.h>
#include <dspxmodel/Param.h>
#include <dspxmodel/ParamMap.h>
#include <dspxmodel/ParamCurveSequence.h>
#include <dspxmodel/SelectionModel.h>

namespace Core {

    ClipPropertyMapper::ClipPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new ClipPropertyMapperPrivate) {
        Q_D(ClipPropertyMapper);
        d->q_ptr = this;
    }

    ClipPropertyMapper::~ClipPropertyMapper() = default;

    dspx::SelectionModel *ClipPropertyMapper::selectionModel() const {
        Q_D(const ClipPropertyMapper);
        return d->selectionModel;
    }

    void ClipPropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(ClipPropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant ClipPropertyMapper::name() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::NameProperty>();
    }

    void ClipPropertyMapper::setName(const QVariant &name) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::NameProperty>(name);
    }

    QVariant ClipPropertyMapper::type() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::TypeProperty>();
    }

    QVariant ClipPropertyMapper::associatedTrack() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::AssociatedTrackProperty>();
    }

    void ClipPropertyMapper::setAssociatedTrack(const QVariant &associatedTrack) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::AssociatedTrackProperty>(associatedTrack);
    }

    QVariant ClipPropertyMapper::mute() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::MuteProperty>();
    }

    void ClipPropertyMapper::setMute(const QVariant &mute) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::MuteProperty>(mute);
    }

    QVariant ClipPropertyMapper::gain() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::GainProperty>();
    }

    void ClipPropertyMapper::setGain(const QVariant &gain) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::GainProperty>(gain);
    }

    QVariant ClipPropertyMapper::pan() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::PanProperty>();
    }

    void ClipPropertyMapper::setPan(const QVariant &pan) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::PanProperty>(pan);
    }

    QVariant ClipPropertyMapper::position() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::PositionProperty>();
    }

    void ClipPropertyMapper::setPosition(const QVariant &position) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::PositionProperty>(position);
    }

    QVariant ClipPropertyMapper::startingOffset() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::StartingOffsetProperty>();
    }

    void ClipPropertyMapper::setStartingOffset(const QVariant &startingOffset) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::StartingOffsetProperty>(startingOffset);
    }

    QVariant ClipPropertyMapper::clipLength() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::ClipLengthProperty>();
    }

    void ClipPropertyMapper::setClipLength(const QVariant &clipLength) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::ClipLengthProperty>(clipLength);
    }

    QVariant ClipPropertyMapper::fullLength() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::FullLengthProperty>();
    }

    void ClipPropertyMapper::setFullLength(const QVariant &fullLength) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::FullLengthProperty>(fullLength);
    }

    void ClipPropertyMapperPrivate::setSelectionModel(dspx::SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        rebuildFromSelection();
    }

    void ClipPropertyMapperPrivate::attachSelectionModel() {
        Q_Q(ClipPropertyMapper);
        if (!selectionModel) {
            return;
        }

        // Listen to selection type changes
        QObject::connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, q, [this] {
            rebindSelectionModels();
        });

        rebindSelectionModels();
    }

    void ClipPropertyMapperPrivate::rebindSelectionModels() {
        Q_Q(ClipPropertyMapper);
        if (!selectionModel) {
            return;
        }

        // Disconnect all existing connections
        unbindSelectionModels();

        const auto selectionType = selectionModel->selectionType();

        // Bind clip selection model
        if (selectionType == dspx::SelectionModel::ST_Clip) {
            clipSelectionModel = selectionModel->clipSelectionModel();
            if (clipSelectionModel) {
                QObject::connect(clipSelectionModel, &dspx::ClipSelectionModel::itemSelected, q, [this](dspx::Clip *clip, bool selected) {
                    handleItemSelected(clip, selected);
                });
            }
        }

        // Bind note selection model
        if (selectionType == dspx::SelectionModel::ST_Note) {
            noteSelectionModel = selectionModel->noteSelectionModel();
            if (noteSelectionModel) {
                QObject::connect(noteSelectionModel, &dspx::NoteSelectionModel::noteSequenceWithSelectedItemsChanged, q, [this] {
                    rebuildFromSelection();
                });
            }
        }

        // Bind anchor node selection model
        if (selectionType == dspx::SelectionModel::ST_AnchorNode) {
            anchorNodeSelectionModel = selectionModel->anchorNodeSelectionModel();
            if (anchorNodeSelectionModel) {
                QObject::connect(anchorNodeSelectionModel, &dspx::AnchorNodeSelectionModel::paramCurveSequenceWithSelectedItemsChanged, q, [this] {
                    rebuildFromSelection();
                });
                QObject::connect(anchorNodeSelectionModel, &dspx::AnchorNodeSelectionModel::paramCurvesAnchorWithSelectedItemsChanged, q, [this] {
                    rebuildFromSelection();
                });
            }
        }

        rebuildFromSelection();
    }

    void ClipPropertyMapperPrivate::unbindSelectionModels() {
        if (clipSelectionModel) {
            QObject::disconnect(clipSelectionModel, nullptr, q_ptr, nullptr);
            clipSelectionModel = nullptr;
        }
        if (noteSelectionModel) {
            QObject::disconnect(noteSelectionModel, nullptr, q_ptr, nullptr);
            noteSelectionModel = nullptr;
        }
        if (anchorNodeSelectionModel) {
            QObject::disconnect(anchorNodeSelectionModel, nullptr, q_ptr, nullptr);
            anchorNodeSelectionModel = nullptr;
        }
    }

    void ClipPropertyMapperPrivate::detachSelectionModel() {
        resetNoteWatchers();
        resetAnchorWatchers();

        if (selectionModel) {
            QObject::disconnect(selectionModel, nullptr, q_ptr, nullptr);
        }

        unbindSelectionModels();
        clear();

        selectionModel = nullptr;
    }

    void ClipPropertyMapperPrivate::rebuildFromSelection() {
        clear();
        rebuildFromClipSelection();
        rebuildFromNoteSelection();
        rebuildFromAnchorSelection();
        refreshCache();
    }

    void ClipPropertyMapperPrivate::rebuildFromClipSelection() {
        if (!clipSelectionModel) {
            return;
        }
        const auto clips = clipSelectionModel->selectedItems();
        for (auto *clip : clips) {
            addItem(clip);
        }
    }

    dspx::Clip *ClipPropertyMapperPrivate::clipFromNoteSequence(dspx::NoteSequence *noteSequence) const {
        if (!noteSequence) {
            return nullptr;
        }
        return noteSequence->singingClip();
    }

    dspx::Clip *ClipPropertyMapperPrivate::clipFromParamCurveSequence(dspx::ParamCurveSequence *paramCurveSequence) const {
        if (!paramCurveSequence) {
            return nullptr;
        }
        if (auto *param = paramCurveSequence->param()) {
            if (auto *paramMap = param->paramMap()) {
                return paramMap->singingClip();
            }
        }
        return nullptr;
    }

    void ClipPropertyMapperPrivate::rebuildFromNoteSelection() {
        resetNoteWatchers();
        if (!noteSelectionModel) {
            return;
        }
        noteSequenceWithSelectedItems = noteSelectionModel->noteSequenceWithSelectedItems();
        setNoteSequenceWatcher(noteSequenceWithSelectedItems);
        if (auto *clip = clipFromNoteSequence(noteSequenceWithSelectedItems)) {
            addItem(clip);
        }
    }

    void ClipPropertyMapperPrivate::rebuildFromAnchorSelection() {
        resetAnchorWatchers();
        if (!anchorNodeSelectionModel) {
            return;
        }
        paramCurveSequenceWithSelectedItems = anchorNodeSelectionModel->paramCurveSequenceWithSelectedItems();
        setAnchorSequenceWatcher(paramCurveSequenceWithSelectedItems);
        setAnchorParamWatcher(paramCurveSequenceWithSelectedItems ? paramCurveSequenceWithSelectedItems->param() : nullptr);
        setAnchorParamMapWatcher(paramWithSelectedItems ? paramWithSelectedItems->paramMap() : nullptr);
        setAnchorSingingClipWatcher(paramMapWithSelectedItems ? paramMapWithSelectedItems->singingClip() : nullptr);
        if (auto *clip = clipFromParamCurveSequence(paramCurveSequenceWithSelectedItems)) {
            addItem(clip);
        }
    }

    void ClipPropertyMapperPrivate::setNoteSequenceWatcher(dspx::NoteSequence *noteSequence) {
        if (!noteSequence) {
            return;
        }
        noteSingingClipWithSelectedItems = noteSequence->singingClip();
    }

    void ClipPropertyMapperPrivate::resetNoteWatchers() {
        if (noteClipSequenceConnection) {
            QObject::disconnect(noteClipSequenceConnection);
        }
        noteClipSequenceConnection = {};
        noteSequenceWithSelectedItems = nullptr;
        noteSingingClipWithSelectedItems = nullptr;
    }

    void ClipPropertyMapperPrivate::setAnchorSequenceWatcher(dspx::ParamCurveSequence *paramCurveSequence) {
        if (!paramCurveSequence) {
            return;
        }
        paramWithSelectedItems = paramCurveSequence->param();
        if (paramWithSelectedItems) {
            anchorParamMapConnection = QObject::connect(paramWithSelectedItems, &dspx::Param::paramMapChanged, q_ptr, [this] {
                rebuildFromSelection();
            });
        }
    }

    void ClipPropertyMapperPrivate::setAnchorParamWatcher(dspx::Param *param) {
        Q_UNUSED(param);
    }

    void ClipPropertyMapperPrivate::setAnchorParamMapWatcher(dspx::ParamMap *paramMap) {
        paramMapWithSelectedItems = paramMap;
        if (!paramMapWithSelectedItems) {
            return;
        }
        anchorSingingClipWithSelectedItems = paramMapWithSelectedItems->singingClip();
    }

    void ClipPropertyMapperPrivate::setAnchorSingingClipWatcher(dspx::SingingClip *singingClip) {
        anchorSingingClipWithSelectedItems = singingClip;
    }

    void ClipPropertyMapperPrivate::resetAnchorWatchers() {
        if (anchorParamMapConnection) {
            QObject::disconnect(anchorParamMapConnection);
        }
        anchorParamMapConnection = {};
        paramCurveSequenceWithSelectedItems = nullptr;
        paramWithSelectedItems = nullptr;
        paramMapWithSelectedItems = nullptr;
        anchorSingingClipWithSelectedItems = nullptr;
    }
}

#include "moc_ClipPropertyMapper.cpp"
