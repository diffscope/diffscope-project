#ifndef DIFFSCOPE_COREPLUGIN_TRACKPROPERTYMAPPER_P_H
#define DIFFSCOPE_COREPLUGIN_TRACKPROPERTYMAPPER_P_H

#include "TrackPropertyMapper.h"

#include <dspxmodel/Track.h>
#include <dspxmodel/TrackControl.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/ParamCurveSequence.h>
#include <dspxmodel/Param.h>
#include <dspxmodel/ParamMap.h>
#include <dspxmodel/AnchorNodeSelectionModel.h>
#include <dspxmodel/ClipSelectionModel.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/TrackSelectionModel.h>

#include <CorePlugin/private/PropertyMapperData_p.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {
    class TrackPropertyMapperPrivate : public PropertyMapperData<
        TrackPropertyMapper,
        TrackPropertyMapperPrivate,
        dspx::Track,
        PropertyMetadata<dspx::Track, &dspx::Track::name, &dspx::Track::setName, decltype(&dspx::Track::nameChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::colorId, &dspx::Track::setColorId, decltype(&dspx::Track::colorIdChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::height, &dspx::Track::setHeight, decltype(&dspx::Track::heightChanged)>,
        PropertyMetadata<dspx::Track, &dspx::TrackControl::mute, &dspx::TrackControl::setMute, decltype(&dspx::TrackControl::muteChanged), &dspx::Track::control>,
        PropertyMetadata<dspx::Track, &dspx::TrackControl::solo, &dspx::TrackControl::setSolo, decltype(&dspx::TrackControl::soloChanged), &dspx::Track::control>,
        PropertyMetadata<dspx::Track, &dspx::TrackControl::record, &dspx::TrackControl::setRecord, decltype(&dspx::TrackControl::recordChanged), &dspx::Track::control>,
        PropertyMetadata<dspx::Track, &dspx::TrackControl::gain, &dspx::TrackControl::setGain, decltype(&dspx::TrackControl::gainChanged), &dspx::Track::control>,
        PropertyMetadata<dspx::Track, &dspx::TrackControl::pan, &dspx::TrackControl::setPan, decltype(&dspx::TrackControl::panChanged), &dspx::Track::control>
    > {
        Q_DECLARE_PUBLIC(TrackPropertyMapper)
    public:
        TrackPropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Track::nameChanged},
            {&dspx::Track::colorIdChanged},
            {&dspx::Track::heightChanged},
            {&dspx::TrackControl::muteChanged},
            {&dspx::TrackControl::soloChanged},
            {&dspx::TrackControl::recordChanged},
            {&dspx::TrackControl::gainChanged},
            {&dspx::TrackControl::panChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::TrackSelectionModel *trackSelectionModel = nullptr;
        dspx::ClipSelectionModel *clipSelectionModel = nullptr;
        dspx::NoteSelectionModel *noteSelectionModel = nullptr;
        dspx::AnchorNodeSelectionModel *anchorNodeSelectionModel = nullptr;

        dspx::NoteSequence *noteSequenceWithSelectedItems = nullptr;
        dspx::ParamCurveSequence *paramCurveSequenceWithSelectedItems = nullptr;
        dspx::Param *paramWithSelectedItems = nullptr;
        dspx::ParamMap *paramMapWithSelectedItems = nullptr;
        dspx::SingingClip *noteSingingClipWithSelectedItems = nullptr;
        dspx::SingingClip *anchorSingingClipWithSelectedItems = nullptr;

        QMetaObject::Connection noteClipSequenceConnection;
        QMetaObject::Connection anchorParamMapConnection;
        QMetaObject::Connection anchorClipSequenceConnection;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();
        void rebindSelectionModels();
        void unbindSelectionModels();

        void rebuildFromSelection();
        void rebuildFromTrackSelection();
        void rebuildFromClipSelection();
        void rebuildFromNoteSelection();
        void rebuildFromAnchorSelection();

        void setNoteSequenceWatcher(dspx::NoteSequence *noteSequence);
        void resetNoteWatchers();

        void setAnchorSequenceWatcher(dspx::ParamCurveSequence *paramCurveSequence);
        void setAnchorParamWatcher(dspx::Param *param);
        void setAnchorParamMapWatcher(dspx::ParamMap *paramMap);
        void setAnchorSingingClipWatcher(dspx::SingingClip *singingClip);
        void resetAnchorWatchers();

        dspx::Track *trackFromNoteSequence(dspx::NoteSequence *noteSequence) const;
        dspx::Track *trackFromParamCurveSequence(dspx::ParamCurveSequence *paramCurveSequence) const;

        enum {
            NameProperty = 0,
            ColorIdProperty = 1,
            HeightProperty = 2,
            MuteProperty = 3,
            SoloProperty = 4,
            RecordProperty = 5,
            GainProperty = 6,
            PanProperty = 7
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(TrackPropertyMapper);
            if constexpr (i == NameProperty) {
                q->nameChanged();
            } else if constexpr (i == ColorIdProperty) {
                q->colorIdChanged();
            } else if constexpr (i == HeightProperty) {
                q->heightChanged();
            } else if constexpr (i == MuteProperty) {
                q->muteChanged();
            } else if constexpr (i == SoloProperty) {
                q->soloChanged();
            } else if constexpr (i == RecordProperty) {
                q->recordChanged();
            } else if constexpr (i == GainProperty) {
                q->gainChanged();
            } else if constexpr (i == PanProperty) {
                q->panChanged();
            }
        }
    };
}

#endif // DIFFSCOPE_COREPLUGIN_TRACKPROPERTYMAPPER_P_H
