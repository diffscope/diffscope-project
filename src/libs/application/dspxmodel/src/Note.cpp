#include "Note.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/note.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/PhonemeInfo.h>
#include <dspxmodel/Pronunciation.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Vibrato.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class NotePrivate {
        Q_DECLARE_PUBLIC(Note)
    public:
        Note *q_ptr;
        ModelPrivate *pModel;

        int centShift;
        int keyNum;
        QString language;
        int length;
        QString lyric;
        PhonemeInfo *phonemes;
        int pos;
        Pronunciation *pronunciation;
        Vibrato *vibrato;
        Workspace *workspace;
        SingingClip *singingClip{};
        bool overlapped{};

        void setCentShiftUnchecked(int centShift_);
        void setCentShift(int centShift_);
        void setKeyNumUnchecked(int keyNum_);
        void setKeyNum(int keyNum_);
        void setLengthUnchecked(int length_);
        void setLength(int length_);
        void setPosUnchecked(int pos_);
        void setPos(int pos_);
    };

    void NotePrivate::setCentShiftUnchecked(int centShift_) {
        Q_Q(Note);
        pModel->strategy->setEntityProperty(q->handle(), ModelStrategy::P_CentShift, centShift_);
    }

    void NotePrivate::setCentShift(int centShift_) {
        Q_Q(Note);
        if (auto engine = qjsEngine(q); engine && (centShift_ < -50 || centShift_ > 50)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("CentShift must be in range [-50, 50]"));
            return;
        }
        setCentShiftUnchecked(centShift_);
    }

    void NotePrivate::setKeyNumUnchecked(int keyNum_) {
        Q_Q(Note);
        pModel->strategy->setEntityProperty(q->handle(), ModelStrategy::P_KeyNumber, keyNum_);
    }

    void NotePrivate::setKeyNum(int keyNum_) {
        Q_Q(Note);
        if (auto engine = qjsEngine(q); engine && (keyNum_ < 0 || keyNum_ > 127)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("KeyNum must be in range [0, 127]"));
            return;
        }
        setKeyNumUnchecked(keyNum_);
    }

    void NotePrivate::setLengthUnchecked(int length_) {
        Q_Q(Note);
        pModel->strategy->setEntityProperty(q->handle(), ModelStrategy::P_Length, length_);
    }

    void NotePrivate::setLength(int length_) {
        Q_Q(Note);
        if (auto engine = qjsEngine(q); engine && length_ < 0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Length must be greater than or equal to 0"));
            return;
        }
        setLengthUnchecked(length_);
    }

    void NotePrivate::setPosUnchecked(int pos_) {
        Q_Q(Note);
        pModel->strategy->setEntityProperty(q->handle(), ModelStrategy::P_Position, pos_);
    }

    void NotePrivate::setPos(int pos_) {
        Q_Q(Note);
        if (auto engine = qjsEngine(q); engine && pos_ < 0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Pos must be greater than or equal to 0"));
            return;
        }
        setPosUnchecked(pos_);
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
        d->phonemes = d->pModel->createObject<PhonemeInfo>(handle);
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
        d->setCentShiftUnchecked(centShift);
    }

    int Note::keyNum() const {
        Q_D(const Note);
        return d->keyNum;
    }

    void Note::setKeyNum(int keyNum) {
        Q_D(Note);
        Q_ASSERT(keyNum >= 0 && keyNum <= 127);
        d->setKeyNumUnchecked(keyNum);
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
        d->setLengthUnchecked(length);
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
        d->setPosUnchecked(pos);
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

    SingingClip *Note::singingClip() const {
        Q_D(const Note);
        return d->singingClip;
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

    void Note::setSingingClip(SingingClip *singingClip) {
        Q_D(Note);
        d->singingClip = singingClip;
        Q_EMIT singingClipChanged();
    }

    void Note::setOverlapped(bool overlapped) {
        Q_D(Note);
        d->overlapped = overlapped;
        Q_EMIT overlappedChanged(overlapped);
    }

}

#include "moc_Note.cpp"
