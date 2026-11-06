// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "KeySignatureAtSpecifiedPositionHelper.h"
#include "KeySignatureAtSpecifiedPositionHelper_p.h"

#include <cstdint>
#include <utility>

#include <dini/engine.h>
#include <dini/query.h>
#include <dini/value.h>
#include <dspxmodelCore/Document.h>
#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>
#include <dspxmodelORM/Model.h>

namespace DspxProject {

    KeySignatureAtSpecifiedPositionHelper::KeySignatureAtSpecifiedPositionHelper(QObject *parent)
        : QObject(parent), d_ptr(new KeySignatureAtSpecifiedPositionHelperPrivate{.q_ptr = this}) {
    }

    KeySignatureAtSpecifiedPositionHelper::~KeySignatureAtSpecifiedPositionHelper() {
        Q_D(KeySignatureAtSpecifiedPositionHelper);
        d->disconnectSequence();
    }

    int KeySignatureAtSpecifiedPositionHelper::position() const {
        Q_D(const KeySignatureAtSpecifiedPositionHelper);
        return d->position;
    }

    void KeySignatureAtSpecifiedPositionHelper::setPosition(int position) {
        Q_D(KeySignatureAtSpecifiedPositionHelper);
        if (d->position == position)
            return;
        d->position = position;
        Q_EMIT positionChanged();
        d->updateKeySignature();
    }

    dspx::KeySignatureSequence *KeySignatureAtSpecifiedPositionHelper::keySignatureSequence() const {
        Q_D(const KeySignatureAtSpecifiedPositionHelper);
        return d->keySignatureSequence;
    }

    void KeySignatureAtSpecifiedPositionHelper::setKeySignatureSequence(dspx::KeySignatureSequence *keySignatureSequence) {
        Q_D(KeySignatureAtSpecifiedPositionHelper);
        if (d->keySignatureSequence == keySignatureSequence)
            return;

        d->disconnectSequence();
        d->keySignatureSequence = keySignatureSequence;
        d->connectSequence();

        Q_EMIT keySignatureSequenceChanged();
        Q_EMIT keySignatureLookupChanged();
        d->updateKeySignature();
    }

    dspx::KeySignature *KeySignatureAtSpecifiedPositionHelper::keySignature() const {
        Q_D(const KeySignatureAtSpecifiedPositionHelper);
        return d->keySignature;
    }

    dspx::KeySignature *KeySignatureAtSpecifiedPositionHelper::keySignatureAt(int position) const {
        Q_D(const KeySignatureAtSpecifiedPositionHelper);
        if (position < 0 || d->keySignatureLookup.isEmpty()) {
            return nullptr;
        }
        auto it = d->keySignatureLookup.upperBound(position);
        if (it == d->keySignatureLookup.cbegin()) {
            return nullptr;
        }
        return (--it).value();
    }

    int KeySignatureAtSpecifiedPositionHelper::mode() const {
        Q_D(const KeySignatureAtSpecifiedPositionHelper);
        return d->keySignature ? d->keySignature->mode() : 0;
    }

    int KeySignatureAtSpecifiedPositionHelper::tonality() const {
        Q_D(const KeySignatureAtSpecifiedPositionHelper);
        return d->keySignature ? d->keySignature->tonality() : 0;
    }

    int KeySignatureAtSpecifiedPositionHelper::accidentalType() const {
        Q_D(const KeySignatureAtSpecifiedPositionHelper);
        return d->keySignature ? d->keySignature->accidentalType() : 0;
    }

    int KeySignatureAtSpecifiedPositionHelper::accidentalTypeAt(int position) const {
        const auto *keySignature = keySignatureAt(position);
        return keySignature ? keySignature->accidentalType() : 0;
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::updateKeySignature() {
        auto *newKeySignature = q_ptr->keySignatureAt(position);

        if (keySignature == newKeySignature)
            return;

        disconnectKeySignature();
        keySignature = newKeySignature;
        connectKeySignature();

        Q_EMIT q_ptr->keySignatureChanged();
        Q_EMIT q_ptr->modeChanged();
        Q_EMIT q_ptr->tonalityChanged();
        Q_EMIT q_ptr->accidentalTypeChanged();
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::rebuildKeySignatureLookup() {
        keySignatureLookup.clear();
        keySignaturePositions.clear();
        if (!keySignatureSequence) {
            return;
        }

        auto *model = keySignatureSequence->model();
        auto *engine = model->document()->engine();
        auto filter = dini::FilterExpression(
            dini::Filter(dini::FieldRef::parent(dspx::Schema::keySignatureParent()),
                         dini::ComparisonOperator::Equal,
                         dini::Value(static_cast<std::uint64_t>(model->handle().d))));
        const auto view = engine->query(dspx::Schema::keySignatureTable(), {
            .filter = std::move(filter),
        });
        const auto snapshots = view.toVector();
        keySignaturePositions.reserve(static_cast<qsizetype>(snapshots.size()));
        for (const auto &snapshot : snapshots) {
            insertKeySignatureIntoLookup(model->find<dspx::KeySignature>(
                dspx::Handle {static_cast<quint64>(snapshot.id)}));
        }
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::insertKeySignatureIntoLookup(dspx::KeySignature *item) {
        if (!item) {
            return;
        }
        const int position = item->position();
        keySignatureLookup.insert(position, item);
        keySignaturePositions.insert(item, position);
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::removeKeySignatureFromLookup(dspx::KeySignature *item) {
        const auto positionIt = keySignaturePositions.find(item);
        if (positionIt == keySignaturePositions.end()) {
            return;
        }
        const int position = positionIt.value();
        keySignaturePositions.erase(positionIt);
        const auto lookupIt = keySignatureLookup.find(position);
        if (lookupIt != keySignatureLookup.end() && lookupIt.value() == item) {
            keySignatureLookup.erase(lookupIt);
        }
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::updateKeySignaturePositionInLookup(dspx::KeySignature *item) {
        removeKeySignatureFromLookup(item);
        insertKeySignatureIntoLookup(item);
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::disconnectSequence() {
        if (keySignatureSequence) {
            QObject::disconnect(keySignatureSequence, nullptr, q_ptr, nullptr);
        }

        for (auto *item : std::as_const(keySignatureLookup)) {
            QObject::disconnect(item, nullptr, q_ptr, nullptr);
        }
        keySignatureLookup.clear();
        keySignaturePositions.clear();
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::connectSequence() {
        if (!keySignatureSequence)
            return;

        rebuildKeySignatureLookup();

        // Connect to sequence signals
        QObject::connect(keySignatureSequence, &dspx::KeySignatureSequence::itemInserted,
                q_ptr, [this](dspx::KeySignature *item) {
                    insertKeySignatureIntoLookup(item);
                    connectSequenceItem(item);
                    Q_EMIT q_ptr->keySignatureLookupChanged();
                    updateKeySignature();
                });

        QObject::connect(keySignatureSequence, &dspx::KeySignatureSequence::itemRemoved,
                q_ptr, [this](dspx::KeySignature *item) {
                    // Disconnect from the removed item
                    QObject::disconnect(item, nullptr, q_ptr, nullptr);
                    removeKeySignatureFromLookup(item);
                    Q_EMIT q_ptr->keySignatureLookupChanged();
                    updateKeySignature();
                });

        for (auto *item : std::as_const(keySignatureLookup)) {
            connectSequenceItem(item);
        }
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::connectSequenceItem(dspx::KeySignature *item) {
        QObject::connect(item, &dspx::KeySignature::positionChanged, q_ptr, [this, item] {
            updateKeySignaturePositionInLookup(item);
            Q_EMIT q_ptr->keySignatureLookupChanged();
            updateKeySignature();
        });
        QObject::connect(item, &dspx::KeySignature::accidentalTypeChanged, q_ptr, [this] {
            Q_EMIT q_ptr->keySignatureLookupChanged();
        });
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::disconnectKeySignature() {
        if (!keySignature)
            return;

        QObject::disconnect(keySignature, &dspx::KeySignature::modeChanged,
                   q_ptr, &KeySignatureAtSpecifiedPositionHelper::modeChanged);
        QObject::disconnect(keySignature, &dspx::KeySignature::tonalityChanged,
                   q_ptr, &KeySignatureAtSpecifiedPositionHelper::tonalityChanged);
        QObject::disconnect(keySignature, &dspx::KeySignature::accidentalTypeChanged,
                   q_ptr, &KeySignatureAtSpecifiedPositionHelper::accidentalTypeChanged);
    }

    void KeySignatureAtSpecifiedPositionHelperPrivate::connectKeySignature() {
        if (!keySignature)
            return;

        QObject::connect(keySignature, &dspx::KeySignature::modeChanged,
                q_ptr, &KeySignatureAtSpecifiedPositionHelper::modeChanged);
        QObject::connect(keySignature, &dspx::KeySignature::tonalityChanged,
                q_ptr, &KeySignatureAtSpecifiedPositionHelper::tonalityChanged);
        QObject::connect(keySignature, &dspx::KeySignature::accidentalTypeChanged,
                q_ptr, &KeySignatureAtSpecifiedPositionHelper::accidentalTypeChanged);
    }

}

#include "moc_KeySignatureAtSpecifiedPositionHelper.cpp"
