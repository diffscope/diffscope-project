#ifndef DIFFSCOPE_COREPLUGIN_NOTEPROPERTYMAPPER_P_H
#define DIFFSCOPE_COREPLUGIN_NOTEPROPERTYMAPPER_P_H

#include "NotePropertyMapper.h"

#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/Pronunciation.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/SelectionModel.h>

#include <CorePlugin/private/PropertyMapperData_p.h>

namespace Core {
    class NotePropertyMapperPrivate : public PropertyMapperData<
        NotePropertyMapper,
        NotePropertyMapperPrivate,
        dspx::Note,
        PropertyMetadata<dspx::Note, &dspx::Note::centShift, &dspx::Note::setCentShift, decltype(&dspx::Note::centShiftChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::keyNum, &dspx::Note::setKeyNum, decltype(&dspx::Note::keyNumChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::language, &dspx::Note::setLanguage, decltype(&dspx::Note::languageChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::length, &dspx::Note::setLength, decltype(&dspx::Note::lengthChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::lyric, &dspx::Note::setLyric, decltype(&dspx::Note::lyricChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Note::pos, &dspx::Note::setPos, decltype(&dspx::Note::posChanged)>,
        PropertyMetadata<dspx::Note, &dspx::Pronunciation::original, &dspx::Pronunciation::setOriginal, decltype(&dspx::Pronunciation::originalChanged), &dspx::Note::pronunciation>,
        PropertyMetadata<dspx::Note, &dspx::Pronunciation::edited, &dspx::Pronunciation::setEdited, decltype(&dspx::Pronunciation::editedChanged), &dspx::Note::pronunciation>,
        PropertyMetadata<dspx::Note,
            [](const dspx::Note *note) { return note->noteSequence() ? note->noteSequence()->singingClip() : nullptr; },
            nullptr,
            decltype(&dspx::Note::noteSequenceChanged)
        >
    > {
        Q_DECLARE_PUBLIC(NotePropertyMapper)
    public:
        NotePropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Note::centShiftChanged},
            {&dspx::Note::keyNumChanged},
            {&dspx::Note::languageChanged},
            {&dspx::Note::lengthChanged},
            {&dspx::Note::lyricChanged},
            {&dspx::Note::posChanged},
            {&dspx::Pronunciation::originalChanged},
            {&dspx::Pronunciation::editedChanged},
            {&dspx::Note::noteSequenceChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::NoteSelectionModel *noteSelectionModel = nullptr;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        enum {
            CentShiftProperty = 0,
            KeyNumProperty = 1,
            LanguageProperty = 2,
            LengthProperty = 3,
            LyricProperty = 4,
            PosProperty = 5,
            PronunciationOriginalProperty = 6,
            PronunciationEditedProperty = 7,
            SingingClipProperty = 8
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(NotePropertyMapper);
            if constexpr (i == CentShiftProperty) {
                q->centShiftChanged();
            } else if constexpr (i == KeyNumProperty) {
                q->keyNumChanged();
            } else if constexpr (i == LanguageProperty) {
                q->languageChanged();
            } else if constexpr (i == LengthProperty) {
                q->lengthChanged();
            } else if constexpr (i == LyricProperty) {
                q->lyricChanged();
            } else if constexpr (i == PosProperty) {
                q->posChanged();
            } else if constexpr (i == PronunciationOriginalProperty) {
                q->pronunciationOriginalChanged();
            } else if constexpr (i == PronunciationEditedProperty) {
                q->pronunciationEditedChanged();
            } else if constexpr (i == SingingClipProperty) {
                q->singingClipChanged();
            }
        }
    };
}

#endif // DIFFSCOPE_COREPLUGIN_NOTEPROPERTYMAPPER_P_H