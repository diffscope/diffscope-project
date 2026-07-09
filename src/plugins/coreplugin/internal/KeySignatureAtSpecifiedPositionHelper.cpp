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

    namespace {

        dspx::KeySignature *keySignatureAt(dspx::KeySignatureSequence *sequence, int position) {
            if (!sequence || position < 0)
                return nullptr;

            auto *model = sequence->model();
            auto *engine = model->document()->engine();
            auto filter = dini::FilterExpression::all({
                dini::FilterExpression(dini::Filter(dini::FieldRef::parent(dspx::Schema::keySignatureParent()),
                                                     dini::ComparisonOperator::Equal,
                                                     dini::Value(static_cast<std::uint64_t>(model->handle().d)))),
                dini::FilterExpression(dini::Filter(dini::FieldRef::column(dspx::Schema::keySignaturePositionColumn()),
                                                     dini::ComparisonOperator::LessOrEqual,
                                                     dini::Value(static_cast<std::int64_t>(position)))),
            });
            const auto view = engine->query(dspx::Schema::keySignatureTable(), {
                .filter = std::move(filter),
                .sortKeys = {
                    dini::SortKey {
                        .field = dini::FieldRef::column(dspx::Schema::keySignaturePositionColumn()),
                        .direction = dini::SortDirection::Descending,
                    },
                },
            });
            const auto snapshots = view.limit(1).toVector();
            return snapshots.empty() ? nullptr : model->find<dspx::KeySignature>(dspx::Handle {static_cast<quint64>(snapshots.front().id)});
        }

    }

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
        auto *newKeySignature = keySignatureAt(m_keySignatureSequence, m_position);

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

        // Disconnect all positionChanged signals from items in the sequence
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
                    // Connect to the newly inserted item's positionChanged signal
                    connect(item, &dspx::KeySignature::positionChanged,
                            this, &KeySignatureAtSpecifiedPositionHelper::updateKeySignature);
                    updateKeySignature();
                });

        connect(m_keySignatureSequence, &dspx::KeySignatureSequence::itemRemoved,
                this, [this](dspx::KeySignature *item) {
                    // Disconnect from the removed item
                    disconnect(item, nullptr, this, nullptr);
                    updateKeySignature();
                });

        // Connect to positionChanged signal of all existing items
        for (auto item : m_keySignatureSequence->asRange()) {
            connect(item, &dspx::KeySignature::positionChanged,
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
