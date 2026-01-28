#include "TempoPropertyMapper.h"
#include "TempoPropertyMapper_p.h"

#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSelectionModel.h>
#include <dspxmodel/SelectionModel.h>

namespace Core {
    TempoPropertyMapper::TempoPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new TempoPropertyMapperPrivate(this)) {
    }

    TempoPropertyMapper::~TempoPropertyMapper() = default;

    dspx::SelectionModel *TempoPropertyMapper::selectionModel() const {
        Q_D(const TempoPropertyMapper);
        return d->selectionModel;
    }

    void TempoPropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(TempoPropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant TempoPropertyMapper::pos() const {
        Q_D(const TempoPropertyMapper);
        return d->cachedPos;
    }

    void TempoPropertyMapper::setPos(const QVariant &pos) {
        Q_D(TempoPropertyMapper);
        if (!d->tempoSelectionModel) {
            return;
        }
        const int value = pos.toInt();
        const auto tempos = d->tempoSelectionModel->selectedItems();
        for (auto *tempo : tempos) {
            tempo->setPos(value);
        }
    }
    QVariant TempoPropertyMapper::value() const {
        Q_D(const TempoPropertyMapper);
        return d->cachedValue;
    }

    void TempoPropertyMapper::setValue(const QVariant &value) {
        Q_D(TempoPropertyMapper);
        if (!d->tempoSelectionModel) {
            return;
        }
        const double val = value.toDouble();
        const auto tempos = d->tempoSelectionModel->selectedItems();
        for (auto *tempo : tempos) {
            tempo->setValue(val);
        }
    }
    TempoPropertyMapperPrivate::TempoPropertyMapperPrivate(TempoPropertyMapper *q)
        : q_ptr(q) {
    }

    void TempoPropertyMapperPrivate::setSelectionModel(dspx::SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        refreshCache();
    }

    void TempoPropertyMapperPrivate::attachSelectionModel() {
        Q_Q(TempoPropertyMapper);
        if (!selectionModel) {
            return;
        }
        tempoSelectionModel = selectionModel->tempoSelectionModel();
        if (!tempoSelectionModel) {
            return;
        }
        QObject::connect(tempoSelectionModel, &dspx::TempoSelectionModel::itemSelected, q, [this](dspx::Tempo *tempo, bool selected) {
            handleItemSelected(tempo, selected);
        });
        const auto existing = tempoSelectionModel->selectedItems();
        for (auto *tempo : existing) {
            addTempo(tempo);
        }
        refreshCache();
    }

    void TempoPropertyMapperPrivate::detachSelectionModel() {
        if (tempoSelectionModel) {
            QObject::disconnect(tempoSelectionModel, nullptr, q_ptr, nullptr);
        }
        clear();
        tempoSelectionModel = nullptr;
        selectionModel = nullptr;
    }

    void TempoPropertyMapperPrivate::handleItemSelected(dspx::Tempo *tempo, bool selected) {
        if (selected) {
            if (!tempoToPos.contains(tempo)) {
                addTempo(tempo);
            }
        } else {
            if (tempoToPos.contains(tempo)) {
                removeTempo(tempo);
            }
        }
        refreshCache();
    }
    void TempoPropertyMapperPrivate::addTempo(dspx::Tempo *tempo) {
        Q_Q(TempoPropertyMapper);
        updatePos(tempo, tempo->pos());
        updateValue(tempo, tempo->value());

        QObject::connect(tempo, &dspx::Tempo::posChanged, q, [this, tempo](int pos) {
            updatePos(tempo, pos);
        });
        QObject::connect(tempo, &dspx::Tempo::valueChanged, q, [this, tempo](double value) {
            updateValue(tempo, value);
        });
        QObject::connect(tempo, &QObject::destroyed, q, [this, tempo] {
            removeTempo(tempo);
            refreshCache();
        });
    }
    void TempoPropertyMapperPrivate::removeTempo(dspx::Tempo *tempo) {
        QObject::disconnect(tempo, nullptr, q_ptr, nullptr);

        if (tempoToPos.contains(tempo)) {
            const int oldPos = tempoToPos.value(tempo);
            posToTempos[oldPos].remove(tempo);
            if (posToTempos[oldPos].isEmpty()) {
                posToTempos.remove(oldPos);
            }
            tempoToPos.remove(tempo);
        }

        if (tempoToValue.contains(tempo)) {
            const double oldValue = tempoToValue.value(tempo);
            valueToTempos[oldValue].remove(tempo);
            if (valueToTempos[oldValue].isEmpty()) {
                valueToTempos.remove(oldValue);
            }
            tempoToValue.remove(tempo);
        }
    }
    void TempoPropertyMapperPrivate::clear() {
        for (auto *tempo : tempoToPos.keys()) {
            QObject::disconnect(tempo, nullptr, q_ptr, nullptr);
        }
        posToTempos.clear();
        valueToTempos.clear();
        tempoToPos.clear();
        tempoToValue.clear();
        cachedPos.clear();
        cachedValue.clear();
    }
    void TempoPropertyMapperPrivate::updatePos(dspx::Tempo *tempo, int pos) {
        if (tempoToPos.contains(tempo)) {
            const int oldPos = tempoToPos.value(tempo);
            if (oldPos == pos) {
                return;
            }
            posToTempos[oldPos].remove(tempo);
            if (posToTempos[oldPos].isEmpty()) {
                posToTempos.remove(oldPos);
            }
        }
        tempoToPos.insert(tempo, pos);
        posToTempos[pos].insert(tempo);
        refreshCache();
    }
    void TempoPropertyMapperPrivate::updateValue(dspx::Tempo *tempo, double value) {
        if (tempoToValue.contains(tempo)) {
            const double oldValue = tempoToValue.value(tempo);
            if (oldValue == value) {
                return;
            }
            valueToTempos[oldValue].remove(tempo);
            if (valueToTempos[oldValue].isEmpty()) {
                valueToTempos.remove(oldValue);
            }
        }
        tempoToValue.insert(tempo, value);
        valueToTempos[value].insert(tempo);
        refreshCache();
    }
    QVariant TempoPropertyMapperPrivate::unifiedPos() const {
        const int count = tempoToPos.size();
        if (count == 0 || posToTempos.size() != 1) {
            return {};
        }
        const auto it = posToTempos.constBegin();
        if (it.value().size() != count) {
            return {};
        }
        return it.key();
    }
    QVariant TempoPropertyMapperPrivate::unifiedValue() const {
        const int count = tempoToValue.size();
        if (count == 0 || valueToTempos.size() != 1) {
            return {};
        }
        const auto it = valueToTempos.constBegin();
        if (it.value().size() != count) {
            return {};
        }
        return it.key();
    }
    void TempoPropertyMapperPrivate::refreshCache() {
        Q_Q(TempoPropertyMapper);
        const QVariant newPos = unifiedPos();
        const QVariant newValue = unifiedValue();
        if (newPos != cachedPos) {
            cachedPos = newPos;
            Q_EMIT q->posChanged();
        }
        if (newValue != cachedValue) {
            cachedValue = newValue;
            Q_EMIT q->valueChanged();
        }
    }
}

#include "moc_TempoPropertyMapper.cpp"
