#include "TrackPropertyMapper.h"
#include "TrackPropertyMapper_p.h"

#include <dspxmodel/Track.h>
#include <dspxmodel/TrackControl.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Param.h>
#include <dspxmodel/ParamMap.h>
#include <dspxmodel/ParamCurveSequence.h>
#include <dspxmodel/SelectionModel.h>

namespace Core {

    TrackPropertyMapper::TrackPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new TrackPropertyMapperPrivate) {
        Q_D(TrackPropertyMapper);
        d->q_ptr = this;
    }

    TrackPropertyMapper::~TrackPropertyMapper() = default;

    dspx::SelectionModel *TrackPropertyMapper::selectionModel() const {
        Q_D(const TrackPropertyMapper);
        return d->selectionModel;
    }

    void TrackPropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(TrackPropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant TrackPropertyMapper::name() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::NameProperty>();
    }

    void TrackPropertyMapper::setName(const QVariant &name) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::NameProperty>(name);
    }

    QVariant TrackPropertyMapper::colorId() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::ColorIdProperty>();
    }

    void TrackPropertyMapper::setColorId(const QVariant &colorId) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::ColorIdProperty>(colorId);
    }

    QVariant TrackPropertyMapper::height() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::HeightProperty>();
    }

    void TrackPropertyMapper::setHeight(const QVariant &height) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::HeightProperty>(height);
    }

    QVariant TrackPropertyMapper::mute() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::MuteProperty>();
    }

    void TrackPropertyMapper::setMute(const QVariant &mute) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::MuteProperty>(mute);
    }

    QVariant TrackPropertyMapper::solo() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::SoloProperty>();
    }

    void TrackPropertyMapper::setSolo(const QVariant &solo) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::SoloProperty>(solo);
    }

    QVariant TrackPropertyMapper::record() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::RecordProperty>();
    }

    void TrackPropertyMapper::setRecord(const QVariant &record) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::RecordProperty>(record);
    }

    QVariant TrackPropertyMapper::gain() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::GainProperty>();
    }

    void TrackPropertyMapper::setGain(const QVariant &gain) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::GainProperty>(gain);
    }

    QVariant TrackPropertyMapper::pan() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::PanProperty>();
    }

    void TrackPropertyMapper::setPan(const QVariant &pan) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::PanProperty>(pan);
    }

    void TrackPropertyMapperPrivate::setSelectionModel(dspx::SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        rebuildFromSelection();
    }

    void TrackPropertyMapperPrivate::attachSelectionModel() {
        Q_Q(TrackPropertyMapper);
        if (!selectionModel) {
            return;
        }

        trackSelectionModel = selectionModel->trackSelectionModel();
        clipSelectionModel = selectionModel->clipSelectionModel();
        noteSelectionModel = selectionModel->noteSelectionModel();
        anchorNodeSelectionModel = selectionModel->anchorNodeSelectionModel();

        if (trackSelectionModel) {
            QObject::connect(trackSelectionModel, &dspx::TrackSelectionModel::itemSelected, q, [this](dspx::Track *track, bool selected) {
                handleItemSelected(track, selected);
            });
        }
        if (clipSelectionModel) {
            QObject::connect(clipSelectionModel, &dspx::ClipSelectionModel::itemSelected, q, [this](dspx::Clip *clip, bool selected) {
                Q_UNUSED(selected);
                Q_UNUSED(clip);
                rebuildFromSelection();
            });
            QObject::connect(clipSelectionModel, &dspx::ClipSelectionModel::clipSequencesWithSelectedItemsChanged, q, [this] {
                rebuildFromSelection();
            });
        }
        if (noteSelectionModel) {
            QObject::connect(noteSelectionModel, &dspx::NoteSelectionModel::itemSelected, q, [this](dspx::Note *note, bool selected) {
                Q_UNUSED(selected);
                Q_UNUSED(note);
                rebuildFromSelection();
            });
            QObject::connect(noteSelectionModel, &dspx::NoteSelectionModel::noteSequenceWithSelectedItemsChanged, q, [this] {
                rebuildFromSelection();
            });
        }
        if (anchorNodeSelectionModel) {
            QObject::connect(anchorNodeSelectionModel, &dspx::AnchorNodeSelectionModel::paramCurveSequenceWithSelectedItemsChanged, q, [this] {
                rebuildFromSelection();
            });
            QObject::connect(anchorNodeSelectionModel, &dspx::AnchorNodeSelectionModel::paramCurvesAnchorWithSelectedItemsChanged, q, [this] {
                rebuildFromSelection();
            });
        }
    }

    void TrackPropertyMapperPrivate::detachSelectionModel() {
        resetNoteWatchers();
        resetAnchorWatchers();

        if (trackSelectionModel) {
            QObject::disconnect(trackSelectionModel, nullptr, q_ptr, nullptr);
        }
        if (clipSelectionModel) {
            QObject::disconnect(clipSelectionModel, nullptr, q_ptr, nullptr);
        }
        if (noteSelectionModel) {
            QObject::disconnect(noteSelectionModel, nullptr, q_ptr, nullptr);
        }
        if (anchorNodeSelectionModel) {
            QObject::disconnect(anchorNodeSelectionModel, nullptr, q_ptr, nullptr);
        }

        clear();

        trackSelectionModel = nullptr;
        clipSelectionModel = nullptr;
        noteSelectionModel = nullptr;
        anchorNodeSelectionModel = nullptr;
        selectionModel = nullptr;
    }

    void TrackPropertyMapperPrivate::rebuildFromSelection() {
        clear();
        rebuildFromTrackSelection();
        rebuildFromClipSelection();
        rebuildFromNoteSelection();
        rebuildFromAnchorSelection();
        refreshCache();
    }

    void TrackPropertyMapperPrivate::rebuildFromTrackSelection() {
        if (!trackSelectionModel) {
            return;
        }
        const auto tracks = trackSelectionModel->selectedItems();
        for (auto *track : tracks) {
            addItem(track);
        }
    }

    void TrackPropertyMapperPrivate::rebuildFromClipSelection() {
        if (!clipSelectionModel) {
            return;
        }
        for (auto *clipSequence : clipSelectionModel->clipSequencesWithSelectedItems()) {
            if (!clipSequence) {
                continue;
            }
            if (auto *track = clipSequence->track()) {
                addItem(track);
            }
        }
    }

    dspx::Track *TrackPropertyMapperPrivate::trackFromNoteSequence(dspx::NoteSequence *noteSequence) const {
        if (!noteSequence) {
            return nullptr;
        }
        if (auto *singingClip = noteSequence->singingClip()) {
            if (auto *clipSequence = singingClip->clipSequence()) {
                return clipSequence->track();
            }
        }
        return nullptr;
    }

    dspx::Track *TrackPropertyMapperPrivate::trackFromParamCurveSequence(dspx::ParamCurveSequence *paramCurveSequence) const {
        if (!paramCurveSequence) {
            return nullptr;
        }
        if (auto *param = paramCurveSequence->param()) {
            if (auto *paramMap = param->paramMap()) {
                if (auto *singingClip = paramMap->singingClip()) {
                    if (auto *clipSequence = singingClip->clipSequence()) {
                        return clipSequence->track();
                    }
                }
            }
        }
        return nullptr;
    }

    void TrackPropertyMapperPrivate::rebuildFromNoteSelection() {
        resetNoteWatchers();
        if (!noteSelectionModel) {
            return;
        }
        noteSequenceWithSelectedItems = noteSelectionModel->noteSequenceWithSelectedItems();
        setNoteSequenceWatcher(noteSequenceWithSelectedItems);
        if (auto *track = trackFromNoteSequence(noteSequenceWithSelectedItems)) {
            addItem(track);
        }
    }

    void TrackPropertyMapperPrivate::rebuildFromAnchorSelection() {
        resetAnchorWatchers();
        if (!anchorNodeSelectionModel) {
            return;
        }
        paramCurveSequenceWithSelectedItems = anchorNodeSelectionModel->paramCurveSequenceWithSelectedItems();
        setAnchorSequenceWatcher(paramCurveSequenceWithSelectedItems);
        setAnchorParamWatcher(paramCurveSequenceWithSelectedItems ? paramCurveSequenceWithSelectedItems->param() : nullptr);
        setAnchorParamMapWatcher(paramWithSelectedItems ? paramWithSelectedItems->paramMap() : nullptr);
        setAnchorSingingClipWatcher(paramMapWithSelectedItems ? paramMapWithSelectedItems->singingClip() : nullptr);
        if (auto *track = trackFromParamCurveSequence(paramCurveSequenceWithSelectedItems)) {
            addItem(track);
        }
    }

    void TrackPropertyMapperPrivate::setNoteSequenceWatcher(dspx::NoteSequence *noteSequence) {
        if (!noteSequence) {
            return;
        }
        noteSingingClipWithSelectedItems = noteSequence->singingClip();
        if (!noteSingingClipWithSelectedItems) {
            return;
        }
        noteClipSequenceConnection = QObject::connect(noteSingingClipWithSelectedItems, &dspx::Clip::clipSequenceChanged, q_ptr, [this] {
            rebuildFromSelection();
        });
    }

    void TrackPropertyMapperPrivate::resetNoteWatchers() {
        if (noteClipSequenceConnection) {
            QObject::disconnect(noteClipSequenceConnection);
        }
        noteClipSequenceConnection = {};
        noteSequenceWithSelectedItems = nullptr;
        noteSingingClipWithSelectedItems = nullptr;
    }

    void TrackPropertyMapperPrivate::setAnchorSequenceWatcher(dspx::ParamCurveSequence *paramCurveSequence) {
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

    void TrackPropertyMapperPrivate::setAnchorParamWatcher(dspx::Param *param) {
        Q_UNUSED(param);
    }

    void TrackPropertyMapperPrivate::setAnchorParamMapWatcher(dspx::ParamMap *paramMap) {
        paramMapWithSelectedItems = paramMap;
        if (!paramMapWithSelectedItems) {
            return;
        }
        anchorSingingClipWithSelectedItems = paramMapWithSelectedItems->singingClip();
        if (!anchorSingingClipWithSelectedItems) {
            return;
        }
        anchorClipSequenceConnection = QObject::connect(anchorSingingClipWithSelectedItems, &dspx::Clip::clipSequenceChanged, q_ptr, [this] {
            rebuildFromSelection();
        });
    }

    void TrackPropertyMapperPrivate::setAnchorSingingClipWatcher(dspx::SingingClip *singingClip) {
        anchorSingingClipWithSelectedItems = singingClip;
    }

    void TrackPropertyMapperPrivate::resetAnchorWatchers() {
        if (anchorParamMapConnection) {
            QObject::disconnect(anchorParamMapConnection);
        }
        if (anchorClipSequenceConnection) {
            QObject::disconnect(anchorClipSequenceConnection);
        }
        anchorParamMapConnection = {};
        anchorClipSequenceConnection = {};
        paramCurveSequenceWithSelectedItems = nullptr;
        paramWithSelectedItems = nullptr;
        paramMapWithSelectedItems = nullptr;
        anchorSingingClipWithSelectedItems = nullptr;
    }
}

#include "moc_TrackPropertyMapper.cpp"
