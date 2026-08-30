// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "KeySignatureAtSpecifiedPositionHelper.h"

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

namespace Core::Internal {

    KeySignatureAtSpecifiedPositionHelper::KeySignatureAtSpecifiedPositionHelper(QObject *parent)
        : QObject(parent) {
    }

    KeySignatureAtSpecifiedPositionHelper::~KeySignatureAtSpecifiedPositionHelper() {
        disconnectSequence();
    }

    int KeySignatureAtSpecifiedPositionHelper::position() const {
        return m_position;
    }

    void KeySignatureAtSpecifiedPositionHelper::setPosition(int position) {
        if (m_position == position)
            return;
        m_position = position;
        emit positionChanged();
        updateKeySignature();
    }

    dspx::KeySignatureSequence *KeySignatureAtSpecifiedPositionHelper::keySignatureSequence() const {
        return m_keySignatureSequence;
    }

    void KeySignatureAtSpecifiedPositionHelper::setKeySignatureSequence(dspx::KeySignatureSequence *keySignatureSequence) {
        if (m_keySignatureSequence == keySignatureSequence)
            return;

        disconnectSequence();
        m_keySignatureSequence = keySignatureSequence;
        connectSequence();

        emit keySignatureSequenceChanged();
        emit keySignatureLookupChanged();
        updateKeySignature();
    }

    dspx::KeySignature *KeySignatureAtSpecifiedPositionHelper::keySignature() const {
        return m_keySignature;
    }

    dspx::KeySignature *KeySignatureAtSpecifiedPositionHelper::keySignatureAt(int position) const {
        if (position < 0 || m_keySignatureLookup.isEmpty()) {
            return nullptr;
        }
        auto it = m_keySignatureLookup.upperBound(position);
        if (it == m_keySignatureLookup.cbegin()) {
            return nullptr;
        }
        return (--it).value();
    }

    int KeySignatureAtSpecifiedPositionHelper::mode() const {
        return m_keySignature ? m_keySignature->mode() : 0;
    }

    int KeySignatureAtSpecifiedPositionHelper::tonality() const {
        return m_keySignature ? m_keySignature->tonality() : 0;
    }

    int KeySignatureAtSpecifiedPositionHelper::accidentalType() const {
        return m_keySignature ? m_keySignature->accidentalType() : 0;
    }

    int KeySignatureAtSpecifiedPositionHelper::accidentalTypeAt(int position) const {
        const auto *keySignature = keySignatureAt(position);
        return keySignature ? keySignature->accidentalType() : 0;
    }

    void KeySignatureAtSpecifiedPositionHelper::updateKeySignature() {
        auto *newKeySignature = keySignatureAt(m_position);

        if (m_keySignature == newKeySignature)
            return;

        disconnectKeySignature();
        m_keySignature = newKeySignature;
        connectKeySignature();

        emit keySignatureChanged();
        emit modeChanged();
        emit tonalityChanged();
        emit accidentalTypeChanged();
    }

    void KeySignatureAtSpecifiedPositionHelper::rebuildKeySignatureLookup() {
        m_keySignatureLookup.clear();
        m_keySignaturePositions.clear();
        if (!m_keySignatureSequence) {
            return;
        }

        auto *model = m_keySignatureSequence->model();
        auto *engine = model->document()->engine();
        auto filter = dini::FilterExpression(
            dini::Filter(dini::FieldRef::parent(dspx::Schema::keySignatureParent()),
                         dini::ComparisonOperator::Equal,
                         dini::Value(static_cast<std::uint64_t>(model->handle().d))));
        const auto view = engine->query(dspx::Schema::keySignatureTable(), {
            .filter = std::move(filter),
        });
        const auto snapshots = view.toVector();
        m_keySignaturePositions.reserve(static_cast<qsizetype>(snapshots.size()));
        for (const auto &snapshot : snapshots) {
            insertKeySignatureIntoLookup(model->find<dspx::KeySignature>(
                dspx::Handle {static_cast<quint64>(snapshot.id)}));
        }
    }

    void KeySignatureAtSpecifiedPositionHelper::insertKeySignatureIntoLookup(dspx::KeySignature *item) {
        if (!item) {
            return;
        }
        const int position = item->position();
        m_keySignatureLookup.insert(position, item);
        m_keySignaturePositions.insert(item, position);
    }

    void KeySignatureAtSpecifiedPositionHelper::removeKeySignatureFromLookup(dspx::KeySignature *item) {
        const auto positionIt = m_keySignaturePositions.find(item);
        if (positionIt == m_keySignaturePositions.end()) {
            return;
        }
        const int position = positionIt.value();
        m_keySignaturePositions.erase(positionIt);
        const auto lookupIt = m_keySignatureLookup.find(position);
        if (lookupIt != m_keySignatureLookup.end() && lookupIt.value() == item) {
            m_keySignatureLookup.erase(lookupIt);
        }
    }

    void KeySignatureAtSpecifiedPositionHelper::updateKeySignaturePositionInLookup(dspx::KeySignature *item) {
        removeKeySignatureFromLookup(item);
        insertKeySignatureIntoLookup(item);
    }

    void KeySignatureAtSpecifiedPositionHelper::disconnectSequence() {
        if (m_keySignatureSequence) {
            disconnect(m_keySignatureSequence, nullptr, this, nullptr);
        }

        for (auto *item : std::as_const(m_keySignatureLookup)) {
            disconnect(item, nullptr, this, nullptr);
        }
        m_keySignatureLookup.clear();
        m_keySignaturePositions.clear();
    }

    void KeySignatureAtSpecifiedPositionHelper::connectSequence() {
        if (!m_keySignatureSequence)
            return;

        rebuildKeySignatureLookup();

        // Connect to sequence signals
        connect(m_keySignatureSequence, &dspx::KeySignatureSequence::itemInserted,
                this, [this](dspx::KeySignature *item) {
                    insertKeySignatureIntoLookup(item);
                    connectSequenceItem(item);
                    emit keySignatureLookupChanged();
                    updateKeySignature();
                });

        connect(m_keySignatureSequence, &dspx::KeySignatureSequence::itemRemoved,
                this, [this](dspx::KeySignature *item) {
                    // Disconnect from the removed item
                    disconnect(item, nullptr, this, nullptr);
                    removeKeySignatureFromLookup(item);
                    emit keySignatureLookupChanged();
                    updateKeySignature();
                });

        for (auto *item : std::as_const(m_keySignatureLookup)) {
            connectSequenceItem(item);
        }
    }

    void KeySignatureAtSpecifiedPositionHelper::connectSequenceItem(dspx::KeySignature *item) {
        connect(item, &dspx::KeySignature::positionChanged, this, [this, item] {
            updateKeySignaturePositionInLookup(item);
            emit keySignatureLookupChanged();
            updateKeySignature();
        });
        connect(item, &dspx::KeySignature::accidentalTypeChanged, this, [this] {
            emit keySignatureLookupChanged();
        });
    }

    void KeySignatureAtSpecifiedPositionHelper::disconnectKeySignature() {
        if (!m_keySignature)
            return;

        disconnect(m_keySignature, &dspx::KeySignature::modeChanged,
                   this, &KeySignatureAtSpecifiedPositionHelper::modeChanged);
        disconnect(m_keySignature, &dspx::KeySignature::tonalityChanged,
                   this, &KeySignatureAtSpecifiedPositionHelper::tonalityChanged);
        disconnect(m_keySignature, &dspx::KeySignature::accidentalTypeChanged,
                   this, &KeySignatureAtSpecifiedPositionHelper::accidentalTypeChanged);
    }

    void KeySignatureAtSpecifiedPositionHelper::connectKeySignature() {
        if (!m_keySignature)
            return;

        connect(m_keySignature, &dspx::KeySignature::modeChanged,
                this, &KeySignatureAtSpecifiedPositionHelper::modeChanged);
        connect(m_keySignature, &dspx::KeySignature::tonalityChanged,
                this, &KeySignatureAtSpecifiedPositionHelper::tonalityChanged);
        connect(m_keySignature, &dspx::KeySignature::accidentalTypeChanged,
                this, &KeySignatureAtSpecifiedPositionHelper::accidentalTypeChanged);
    }

}

#include "moc_KeySignatureAtSpecifiedPositionHelper.cpp"
