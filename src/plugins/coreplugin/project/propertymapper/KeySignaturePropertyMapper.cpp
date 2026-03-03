#include "KeySignaturePropertyMapper.h"
#include "KeySignaturePropertyMapper_p.h"

#include <dspxmodel/KeySignature.h>
#include <dspxmodel/KeySignatureSelectionModel.h>
#include <dspxmodel/SelectionModel.h>

namespace Core {

    KeySignaturePropertyMapper::KeySignaturePropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new KeySignaturePropertyMapperPrivate) {
        Q_D(KeySignaturePropertyMapper);
        d->q_ptr = this;
    }

    KeySignaturePropertyMapper::~KeySignaturePropertyMapper() = default;

    dspx::SelectionModel *KeySignaturePropertyMapper::selectionModel() const {
        Q_D(const KeySignaturePropertyMapper);
        return d->selectionModel;
    }

    void KeySignaturePropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(KeySignaturePropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant KeySignaturePropertyMapper::pos() const {
        Q_D(const KeySignaturePropertyMapper);
        return d->value<KeySignaturePropertyMapperPrivate::PosProperty>();
    }

    void KeySignaturePropertyMapper::setPos(const QVariant &pos) {
        Q_D(KeySignaturePropertyMapper);
        d->setValue<KeySignaturePropertyMapperPrivate::PosProperty>(pos);
    }

    QVariant KeySignaturePropertyMapper::mode() const {
        Q_D(const KeySignaturePropertyMapper);
        return d->value<KeySignaturePropertyMapperPrivate::ModeProperty>();
    }

    void KeySignaturePropertyMapper::setMode(const QVariant &mode) {
        Q_D(KeySignaturePropertyMapper);
        d->setValue<KeySignaturePropertyMapperPrivate::ModeProperty>(mode);
    }

    QVariant KeySignaturePropertyMapper::tonality() const {
        Q_D(const KeySignaturePropertyMapper);
        return d->value<KeySignaturePropertyMapperPrivate::TonalityProperty>();
    }

    void KeySignaturePropertyMapper::setTonality(const QVariant &tonality) {
        Q_D(KeySignaturePropertyMapper);
        d->setValue<KeySignaturePropertyMapperPrivate::TonalityProperty>(tonality);
    }

    QVariant KeySignaturePropertyMapper::accidentalType() const {
        Q_D(const KeySignaturePropertyMapper);
        return d->value<KeySignaturePropertyMapperPrivate::AccidentalTypeProperty>();
    }

    void KeySignaturePropertyMapper::setAccidentalType(const QVariant &accidentalType) {
        Q_D(KeySignaturePropertyMapper);
        d->setValue<KeySignaturePropertyMapperPrivate::AccidentalTypeProperty>(accidentalType);
    }

    void KeySignaturePropertyMapperPrivate::setSelectionModel(dspx::SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        refreshCache();
    }

    void KeySignaturePropertyMapperPrivate::attachSelectionModel() {
        Q_Q(KeySignaturePropertyMapper);
        if (!selectionModel) {
            return;
        }
        keySignatureSelectionModel = selectionModel->keySignatureSelectionModel();
        if (!keySignatureSelectionModel) {
            return;
        }
        QObject::connect(keySignatureSelectionModel, &dspx::KeySignatureSelectionModel::itemSelected, q, [this](dspx::KeySignature *keySignature, bool selected) {
            handleItemSelected(keySignature, selected);
        });
        const auto existing = keySignatureSelectionModel->selectedItems();
        for (auto *keySignature : existing) {
            addItem(keySignature);
        }
        refreshCache();
    }

    void KeySignaturePropertyMapperPrivate::detachSelectionModel() {
        if (keySignatureSelectionModel) {
            QObject::disconnect(keySignatureSelectionModel, nullptr, q_ptr, nullptr);
        }
        clear();
        keySignatureSelectionModel = nullptr;
        selectionModel = nullptr;
    }
}

#include "moc_KeySignaturePropertyMapper.cpp"
