#include "KeySignatureSequence.h"
#include "KeySignatureSequence_p.h"

#include <QJSEngine>
#include <QJsonArray>
#include <QJsonObject>

#include <nlohmann/json.hpp>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/KeySignature.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    KeySignatureSequence::KeySignatureSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new KeySignatureSequencePrivate) {
        Q_D(KeySignatureSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_KeySignatures);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);

        d->init(model->strategy()->getEntitiesFromSequenceContainer(handle));
    }

    KeySignatureSequence::~KeySignatureSequence() = default;

    int KeySignatureSequence::size() const {
        Q_D(const KeySignatureSequence);
        return d->container.size();
    }

    KeySignature *KeySignatureSequence::firstItem() const {
        Q_D(const KeySignatureSequence);
        return d->firstItem;
    }

    KeySignature *KeySignatureSequence::lastItem() const {
        Q_D(const KeySignatureSequence);
        return d->lastItem;
    }

    KeySignature *KeySignatureSequence::previousItem(KeySignature *item) const {
        Q_D(const KeySignatureSequence);
        return d->container.previousItem(item);
    }

    KeySignature *KeySignatureSequence::nextItem(KeySignature *item) const {
        Q_D(const KeySignatureSequence);
        return d->container.nextItem(item);
    }

    QList<KeySignature *> KeySignatureSequence::slice(int position, int length) const {
        Q_D(const KeySignatureSequence);
        return d->container.slice(position, length);
    }

    KeySignature *KeySignatureSequence::itemAt(int position) const {
        Q_D(const KeySignatureSequence);
        auto it = d->container.m_items.upper_bound(position);
        if (it != d->container.m_items.begin()) {
            --it;
            return it->second;
        }
        return nullptr;
    }

    bool KeySignatureSequence::contains(KeySignature *item) const {
        Q_D(const KeySignatureSequence);
        return d->container.contains(item);
    }

    bool KeySignatureSequence::insertItem(KeySignature *item) {
        Q_D(KeySignatureSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool KeySignatureSequence::removeItem(KeySignature *item) {
        Q_D(KeySignatureSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    nlohmann::json KeySignatureSequence::toOpenDspx() const {
        Q_D(const KeySignatureSequence);
        nlohmann::json ret = nlohmann::json::array();
        for (const auto &[_, item] : d->container.m_items) {
            ret.push_back(item->toOpenDspx());
        }
        return ret;
    }

    void KeySignatureSequence::fromOpenDspx(const nlohmann::json &keySignatures) {
        while (size()) {
            removeItem(firstItem());
        }
        if (!keySignatures.is_array())
            return;
        for (const auto &value : keySignatures) {
            auto item = model()->createKeySignature();
            item->fromOpenDspx(value);
            insertItem(item);
        }
    }

    void KeySignatureSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(KeySignatureSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void KeySignatureSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(KeySignatureSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

}

#include "moc_KeySignatureSequence.cpp"
