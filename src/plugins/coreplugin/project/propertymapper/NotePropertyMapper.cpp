#include "NotePropertyMapper.h"
#include "NotePropertyMapper_p.h"

#include <dspxmodel/SelectionModel.h>

namespace Core {

    NotePropertyMapper::NotePropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new NotePropertyMapperPrivate) {
        Q_D(NotePropertyMapper);
        d->q_ptr = this;
    }

    NotePropertyMapper::~NotePropertyMapper() = default;

    dspx::SelectionModel *NotePropertyMapper::selectionModel() const {
        Q_D(const NotePropertyMapper);
        return d->selectionModel;
    }

    void NotePropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(NotePropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant NotePropertyMapper::centShift() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::CentShiftProperty>();
    }

    void NotePropertyMapper::setCentShift(const QVariant &centShift) {
        Q_D(NotePropertyMapper);
        d->setValue<NotePropertyMapperPrivate::CentShiftProperty>(centShift);
    }

    QVariant NotePropertyMapper::keyNum() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::KeyNumProperty>();
    }

    void NotePropertyMapper::setKeyNum(const QVariant &keyNum) {
        Q_D(NotePropertyMapper);
        d->setValue<NotePropertyMapperPrivate::KeyNumProperty>(keyNum);
    }

    QVariant NotePropertyMapper::language() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::LanguageProperty>();
    }

    void NotePropertyMapper::setLanguage(const QVariant &language) {
        Q_D(NotePropertyMapper);
        d->setValue<NotePropertyMapperPrivate::LanguageProperty>(language);
    }

    QVariant NotePropertyMapper::length() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::LengthProperty>();
    }

    void NotePropertyMapper::setLength(const QVariant &length) {
        Q_D(NotePropertyMapper);
        d->setValue<NotePropertyMapperPrivate::LengthProperty>(length);
    }

    QVariant NotePropertyMapper::lyric() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::LyricProperty>();
    }

    void NotePropertyMapper::setLyric(const QVariant &lyric) {
        Q_D(NotePropertyMapper);
        d->setValue<NotePropertyMapperPrivate::LyricProperty>(lyric);
    }

    QVariant NotePropertyMapper::pos() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::PosProperty>();
    }

    void NotePropertyMapper::setPos(const QVariant &pos) {
        Q_D(NotePropertyMapper);
        d->setValue<NotePropertyMapperPrivate::PosProperty>(pos);
    }

    QVariant NotePropertyMapper::pronunciationOriginal() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::PronunciationOriginalProperty>();
    }

    void NotePropertyMapper::setPronunciationOriginal(const QVariant &pronunciationOriginal) {
        Q_D(NotePropertyMapper);
        d->setValue<NotePropertyMapperPrivate::PronunciationOriginalProperty>(pronunciationOriginal);
    }

    QVariant NotePropertyMapper::pronunciationEdited() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::PronunciationEditedProperty>();
    }

    void NotePropertyMapper::setPronunciationEdited(const QVariant &pronunciationEdited) {
        Q_D(NotePropertyMapper);
        d->setValue<NotePropertyMapperPrivate::PronunciationEditedProperty>(pronunciationEdited);
    }

    QVariant NotePropertyMapper::singingClip() const {
        Q_D(const NotePropertyMapper);
        return d->value<NotePropertyMapperPrivate::SingingClipProperty>();
    }

    void NotePropertyMapperPrivate::setSelectionModel(dspx::SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        refreshCache();
    }

    void NotePropertyMapperPrivate::attachSelectionModel() {
        Q_Q(NotePropertyMapper);
        if (!selectionModel) {
            return;
        }
        noteSelectionModel = selectionModel->noteSelectionModel();
        if (!noteSelectionModel) {
            return;
        }
        QObject::connect(noteSelectionModel, &dspx::NoteSelectionModel::itemSelected, q, [this](dspx::Note *note, bool selected) {
            handleItemSelected(note, selected);
        });
        const auto existing = noteSelectionModel->selectedItems();
        for (auto *note : existing) {
            addItem(note);
        }
        refreshCache();
    }

    void NotePropertyMapperPrivate::detachSelectionModel() {
        if (noteSelectionModel) {
            QObject::disconnect(noteSelectionModel, nullptr, q_ptr, nullptr);
        }
        clear();
        noteSelectionModel = nullptr;
        selectionModel = nullptr;
    }
}

#include "moc_NotePropertyMapper.cpp"