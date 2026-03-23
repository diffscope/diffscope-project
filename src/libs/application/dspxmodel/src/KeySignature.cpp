#include "KeySignature.h"
#include "KeySignature_p.h"

#include <QVariant>
#include <QJsonObject>

#include <nlohmann/json.hpp>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/KeySignatureSequence.h>

namespace dspx {

    void KeySignaturePrivate::setKeySignatureSequence(KeySignature *item, KeySignatureSequence *keySignatureSequence) {
        auto d = item->d_func();
        if (d->keySignatureSequence != keySignatureSequence) {
            d->keySignatureSequence = keySignatureSequence;
            Q_EMIT item->keySignatureSequenceChanged();
        }
    }

    void KeySignaturePrivate::setPreviousItem(KeySignature *item, KeySignature *previousItem) {
        auto d = item->d_func();
        if (d->previousItem != previousItem) {
            d->previousItem = previousItem;
            Q_EMIT item->previousItemChanged();
        }
    }

    void KeySignaturePrivate::setNextItem(KeySignature *item, KeySignature *nextItem) {
        auto d = item->d_func();
        if (d->nextItem != nextItem) {
            d->nextItem = nextItem;
            Q_EMIT item->nextItemChanged();
        }
    }

    KeySignature::KeySignature(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new KeySignaturePrivate) {
        Q_D(KeySignature);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_KeySignature);
        d->q_ptr = this;
        d->pos = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Position).toInt();
        d->mode = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Mode).toInt();
        d->tonality = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Tonality).toInt();
        d->accidentalType = static_cast<AccidentalType>(model->strategy()->getEntityProperty(handle, ModelStrategy::P_AccidentalType).toInt());
    }

    KeySignature::~KeySignature() = default;

    int KeySignature::pos() const {
        Q_D(const KeySignature);
        return d->pos;
    }

    void KeySignature::setPos(int pos) {
        Q_D(KeySignature);
        Q_ASSERT(pos >= 0);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Position, pos);
    }

    int KeySignature::mode() const {
        Q_D(const KeySignature);
        return d->mode;
    }

    void KeySignature::setMode(int mode) {
        Q_D(KeySignature);
        Q_ASSERT(mode >= 0 && mode <= 4095);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Mode, mode);
    }

    int KeySignature::tonality() const {
        Q_D(const KeySignature);
        return d->tonality;
    }

    void KeySignature::setTonality(int tonality) {
        Q_D(KeySignature);
        Q_ASSERT(tonality >= 0 && tonality <= 11);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Tonality, tonality);
    }

    KeySignature::AccidentalType KeySignature::accidentalType() const {
        Q_D(const KeySignature);
        return d->accidentalType;
    }

    void KeySignature::setAccidentalType(AccidentalType accidentalType) {
        Q_D(KeySignature);
        Q_ASSERT(accidentalType == Flat || accidentalType == Sharp);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_AccidentalType, static_cast<int>(accidentalType));
    }

    KeySignatureSequence *KeySignature::keySignatureSequence() const {
        Q_D(const KeySignature);
        return d->keySignatureSequence;
    }

    KeySignature *KeySignature::previousItem() const {
        Q_D(const KeySignature);
        return d->previousItem;
    }

    KeySignature *KeySignature::nextItem() const {
        Q_D(const KeySignature);
        return d->nextItem;
    }

    nlohmann::json KeySignature::toOpenDspx() const {
        return nlohmann::json::object({
            {"pos", pos()},
            {"mode", mode()},
            {"tonality", tonality()},
            {"accidentalType", accidentalType()}
        });
    }

    void KeySignature::fromOpenDspx(const nlohmann::json &keySignature) {
        try {
            auto pos = keySignature.at("pos").get<int>();
            pos = qMax(0, pos);
            setPos(pos);
            auto mode = keySignature.at("mode").get<int>();
            if (mode < 0 || mode > 4095) {
                mode = 0;
            }
            setMode(mode);
            auto tonality = keySignature.at("tonality").get<int>();
            if (tonality < 0 || tonality > 11) {
                tonality = 0;
            }
            setTonality(tonality);
            auto accidentalType = keySignature.at("accidentalType").get<int>();
            if (accidentalType != Flat && accidentalType != Sharp) {
                accidentalType = Flat;
            }
            setAccidentalType(static_cast<AccidentalType>(accidentalType));
        } catch (const nlohmann::json::exception &) {
            // ignore
        }
    }

    void KeySignature::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(KeySignature);
        switch (property) {
            case ModelStrategy::P_Position: {
                d->pos = value.toInt();
                Q_EMIT posChanged(d->pos);
                break;
            }
            case ModelStrategy::P_Mode: {
                d->mode = value.toInt();
                Q_EMIT modeChanged(d->mode);
                break;
            }
            case ModelStrategy::P_Tonality: {
                d->tonality = value.toInt();
                Q_EMIT tonalityChanged(d->tonality);
                break;
            }
            case ModelStrategy::P_AccidentalType: {
                d->accidentalType = static_cast<AccidentalType>(value.toInt());
                Q_EMIT accidentalTypeChanged(d->accidentalType);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_KeySignature.cpp"
