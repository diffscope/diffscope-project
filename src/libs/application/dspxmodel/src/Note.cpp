#include "Note.h"
#include "Note_p.h"
#include <QVariant>

#include <opendspx/note.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/PhonemeInfo.h>
#include <dspxmodel/Pronunciation.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/Vibrato.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {



    void NotePrivate::setOverlapped(Note *item, bool overlapped) {
        auto d = item->d_func();
        if (d->overlapped != overlapped) {
            d->overlapped = overlapped;
            Q_EMIT item->overlappedChanged(overlapped);
        }
    }

    void NotePrivate::setNoteSequence(Note *item, NoteSequence *noteSequence) {
        auto d = item->d_func();
        if (d->noteSequence != noteSequence) {
            d->noteSequence = noteSequence;
            Q_EMIT item->noteSequenceChanged();
        }
    }

    Note::Note(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new NotePrivate) {
        Q_D(Note);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Note);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->centShift = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_CentShift).toInt();
        d->keyNum = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_KeyNumber).toInt();
        d->language = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Language).toString();
        d->length = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Length).toInt();
        d->lyric = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Text).toString();
        d->pos = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Position).toInt();
        d->phonemes = d->pModel->createObject<PhonemeInfo>(this, handle);
        d->pronunciation = d->pModel->createObject<Pronunciation>(handle);
        d->vibrato = d->pModel->createObject<Vibrato>(handle);
        d->workspace = d->pModel->createObject<Workspace>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Workspace));
    }

    Note::~Note() = default;

    int Note::centShift() const {
        Q_D(const Note);
        return d->centShift;
    }

    void Note::setCentShift(int centShift) {
        Q_D(Note);
        Q_ASSERT(centShift >= -50 && centShift <= 50);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_CentShift, centShift);
    }

    int Note::keyNum() const {
        Q_D(const Note);
        return d->keyNum;
    }

    void Note::setKeyNum(int keyNum) {
        Q_D(Note);
        Q_ASSERT(keyNum >= 0 && keyNum <= 127);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_KeyNumber, keyNum);
    }

    QString Note::language() const {
        Q_D(const Note);
        return d->language;
    }

    void Note::setLanguage(const QString &language) {
        Q_D(Note);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Language, language);
    }

    int Note::length() const {
        Q_D(const Note);
        return d->length;
    }

    void Note::setLength(int length) {
        Q_D(Note);
        Q_ASSERT(length >= 0);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Length, length);
    }

    QString Note::lyric() const {
        Q_D(const Note);
        return d->lyric;
    }

    void Note::setLyric(const QString &lyric) {
        Q_D(Note);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Text, lyric);
    }

    PhonemeInfo *Note::phonemes() const {
        Q_D(const Note);
        return d->phonemes;
    }

    int Note::pos() const {
        Q_D(const Note);
        return d->pos;
    }

    void Note::setPos(int pos) {
        Q_D(Note);
        Q_ASSERT(pos >= 0);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Position, pos);
    }

    Pronunciation *Note::pronunciation() const {
        Q_D(const Note);
        return d->pronunciation;
    }

    Vibrato *Note::vibrato() const {
        Q_D(const Note);
        return d->vibrato;
    }

    Workspace *Note::workspace() const {
        Q_D(const Note);
        return d->workspace;
    }

    NoteSequence *Note::noteSequence() const {
        Q_D(const Note);
        return d->noteSequence;
    }

    bool Note::isOverlapped() const {
        Q_D(const Note);
        return d->overlapped;
    }

    QDspx::Note Note::toQDspx() const {
        return {
            .pos = pos(),
            .length = length(),
            .keyNum = keyNum(),
            .centShift = centShift(),
            .language = language(),
            .lyric = lyric(),
            .pronunciation = pronunciation()->toQDspx(),
            .phonemes = phonemes()->toQDspx(),
            .vibrato = vibrato()->toQDspx(),
            .workspace = workspace()->toQDspx(),
        };
    }

    void Note::fromQDspx(const QDspx::Note &note) {
        setPos(note.pos);
        setLength(note.length);
        setKeyNum(note.keyNum);
        setCentShift(note.centShift);
        setLanguage(note.language);
        setLyric(note.lyric);
        pronunciation()->fromQDspx(note.pronunciation);
        phonemes()->fromQDspx(note.phonemes);
        vibrato()->fromQDspx(note.vibrato);
        workspace()->fromQDspx(note.workspace);
    }

    void Note::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Note);
        switch (property) {
            case ModelStrategy::P_CentShift: {
                d->centShift = value.toInt();
                Q_EMIT centShiftChanged(d->centShift);
                break;
            }
            case ModelStrategy::P_KeyNumber: {
                d->keyNum = value.toInt();
                Q_EMIT keyNumChanged(d->keyNum);
                break;
            }
            case ModelStrategy::P_Language: {
                d->language = value.toString();
                Q_EMIT languageChanged(d->language);
                break;
            }
            case ModelStrategy::P_Length: {
                d->length = value.toInt();
                Q_EMIT lengthChanged(d->length);
                break;
            }
            case ModelStrategy::P_Text: {
                d->lyric = value.toString();
                Q_EMIT lyricChanged(d->lyric);
                break;
            }
            case ModelStrategy::P_Position: {
                d->pos = value.toInt();
                Q_EMIT posChanged(d->pos);
                break;
            }
            case ModelStrategy::P_PronunciationOriginal:
            case ModelStrategy::P_PronunciationEdited: {
                ModelPrivate::proxySetEntityPropertyNotify(d->pronunciation, property, value);
                break;
            }
            case ModelStrategy::P_VibratoAmplitude:
            case ModelStrategy::P_VibratoEnd:
            case ModelStrategy::P_VibratoFrequency:
            case ModelStrategy::P_VibratoOffset:
            case ModelStrategy::P_VibratoPhase:
            case ModelStrategy::P_VibratoStart: {
                ModelPrivate::proxySetEntityPropertyNotify(d->vibrato, property, value);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Note.cpp"
