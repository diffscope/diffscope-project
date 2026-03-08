#include "KeySignatureAtSpecifiedPositionHelper.h"

#include <dspxmodel/KeySignature.h>
#include <dspxmodel/KeySignatureSequence.h>

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
        updateKeySignature();
    }

    dspx::KeySignature *KeySignatureAtSpecifiedPositionHelper::keySignature() const {
        return m_keySignature;
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

    void KeySignatureAtSpecifiedPositionHelper::updateKeySignature() {
        dspx::KeySignature *newKeySignature = nullptr;
        if (m_keySignatureSequence) {
            newKeySignature = m_keySignatureSequence->itemAt(m_position);
        }

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

    void KeySignatureAtSpecifiedPositionHelper::disconnectSequence() {
        if (!m_keySignatureSequence)
            return;

        // Disconnect all signals from the sequence
        disconnect(m_keySignatureSequence, nullptr, this, nullptr);

        // Disconnect all posChanged signals from items in the sequence
        for (auto item : m_keySignatureSequence->asRange()) {
            disconnect(item, nullptr, this, nullptr);
        }
    }

    void KeySignatureAtSpecifiedPositionHelper::connectSequence() {
        if (!m_keySignatureSequence)
            return;

        // Connect to sequence signals
        connect(m_keySignatureSequence, &dspx::KeySignatureSequence::itemInserted,
                this, [this](dspx::KeySignature *item) {
                    // Connect to the newly inserted item's posChanged signal
                    connect(item, &dspx::KeySignature::posChanged,
                            this, &KeySignatureAtSpecifiedPositionHelper::updateKeySignature);
                    updateKeySignature();
                });

        connect(m_keySignatureSequence, &dspx::KeySignatureSequence::itemRemoved,
                this, [this](dspx::KeySignature *item) {
                    // Disconnect from the removed item
                    disconnect(item, nullptr, this, nullptr);
                    updateKeySignature();
                });

        // Connect to posChanged signal of all existing items
        for (auto item : m_keySignatureSequence->asRange()) {
            connect(item, &dspx::KeySignature::posChanged,
                    this, &KeySignatureAtSpecifiedPositionHelper::updateKeySignature);
        }
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
