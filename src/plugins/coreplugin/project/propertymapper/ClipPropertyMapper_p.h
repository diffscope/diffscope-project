#ifndef DIFFSCOPE_COREPLUGIN_CLIPPROPERTYMAPPER_P_H
#define DIFFSCOPE_COREPLUGIN_CLIPPROPERTYMAPPER_P_H

#include "ClipPropertyMapper.h"

#include <dspxmodel/AnchorNode.h>
#include <dspxmodel/AnchorNodeSelectionModel.h>
#include <dspxmodel/BusControl.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/ClipSelectionModel.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/Param.h>
#include <dspxmodel/ParamCurveSequence.h>
#include <dspxmodel/ParamMap.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Track.h>

#include <CorePlugin/private/PropertyMapperData_p.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {
    class ClipPropertyMapperPrivate : public PropertyMapperData<
        ClipPropertyMapper,
        ClipPropertyMapperPrivate,
        dspx::Clip,
        PropertyMetadata<dspx::Clip, &dspx::Clip::name, &dspx::Clip::setName, decltype(&dspx::Clip::nameChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::type, nullptr, nullptr_t>,
        PropertyMetadata<dspx::Clip,
            [](const dspx::Clip *clip) { return clip->clipSequence()->track(); },
            [](dspx::Clip *clip, dspx::Track *track) { if (track) clip->clipSequence()->moveToAnotherClipSequence(clip, track->clips()); },
            decltype(&dspx::Clip::clipSequenceChanged)
        >,
        PropertyMetadata<dspx::Clip, &dspx::BusControl::mute, &dspx::BusControl::setMute, decltype(&dspx::BusControl::muteChanged), &dspx::Clip::control>,
        PropertyMetadata<dspx::Clip, &dspx::BusControl::gain, &dspx::BusControl::setGain, decltype(&dspx::BusControl::gainChanged), &dspx::Clip::control>,
        PropertyMetadata<dspx::Clip, &dspx::BusControl::pan, &dspx::BusControl::setPan, decltype(&dspx::BusControl::panChanged), &dspx::Clip::control>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::position, [](dspx::Clip *clip, int position) {
            clip->time()->setStart(position - clip->time()->clipStart());
        }, decltype(&dspx::Clip::positionChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::ClipTime::clipStart, &dspx::ClipTime::setClipStart, decltype(&dspx::ClipTime::clipStartChanged), &dspx::Clip::time>,
        PropertyMetadata<dspx::Clip, &dspx::ClipTime::clipLen, &dspx::ClipTime::setClipLen, decltype(&dspx::ClipTime::clipLenChanged), &dspx::Clip::time>,
        PropertyMetadata<dspx::Clip, &dspx::ClipTime::length, &dspx::ClipTime::setLength, decltype(&dspx::ClipTime::lengthChanged), &dspx::Clip::time>
    > {
        Q_DECLARE_PUBLIC(ClipPropertyMapper)
    public:
        ClipPropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Clip::nameChanged},
            {nullptr},
            {&dspx::Clip::clipSequenceChanged},
            {&dspx::BusControl::muteChanged},
            {&dspx::BusControl::gainChanged},
            {&dspx::BusControl::panChanged},
            {&dspx::Clip::positionChanged},
            {&dspx::ClipTime::clipStartChanged},
            {&dspx::ClipTime::clipLenChanged},
            {&dspx::ClipTime::lengthChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
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

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        void rebuildFromSelection();
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

        dspx::Clip *clipFromNoteSequence(dspx::NoteSequence *noteSequence) const;
        dspx::Clip *clipFromParamCurveSequence(dspx::ParamCurveSequence *paramCurveSequence) const;

        enum {
            NameProperty = 0,
            TypeProperty = 1,
            AssociatedTrackProperty = 2,
            MuteProperty = 3,
            GainProperty = 4,
            PanProperty = 5,
            PositionProperty = 6,
            StartingOffsetProperty = 7,
            ClipLengthProperty = 8,
            FullLengthProperty = 9
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(ClipPropertyMapper);
            if constexpr (i == NameProperty) {
                q->nameChanged();
            } else if constexpr (i == TypeProperty) {
                q->typeChanged();
            } else if constexpr (i == AssociatedTrackProperty) {
                q->associatedTrackChanged();
            } else if constexpr (i == MuteProperty) {
                q->muteChanged();
            } else if constexpr (i == GainProperty) {
                q->gainChanged();
            } else if constexpr (i == PanProperty) {
                q->panChanged();
            } else if constexpr (i == PositionProperty) {
                q->positionChanged();
            } else if constexpr (i == StartingOffsetProperty) {
                q->startingOffsetChanged();
            } else if constexpr (i == ClipLengthProperty) {
                q->clipLengthChanged();
            } else if constexpr (i == FullLengthProperty) {
                q->fullLengthChanged();
            }
        }
    };
}

#endif // DIFFSCOPE_COREPLUGIN_CLIPPROPERTYMAPPER_P_H
