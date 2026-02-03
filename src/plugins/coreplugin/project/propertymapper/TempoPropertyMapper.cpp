#include "TempoPropertyMapper.h"
#include "TempoPropertyMapper_p.h"

#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSelectionModel.h>
#include <dspxmodel/SelectionModel.h>

namespace Core {

    TempoPropertyMapper::TempoPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new TempoPropertyMapperPrivate) {
        Q_D(TempoPropertyMapper);
        d->q_ptr = this;
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
        return d->value<TempoPropertyMapperPrivate::PosProperty>();
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
        return d->value<TempoPropertyMapperPrivate::ValueProperty>();
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
            addItem(tempo);
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
}

#include "moc_TempoPropertyMapper.cpp"
