#include "LabelPropertyMapper.h"
#include "LabelPropertyMapper_p.h"

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/SelectionModel.h>

namespace Core {

    LabelPropertyMapper::LabelPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new LabelPropertyMapperPrivate) {
        Q_D(LabelPropertyMapper);
        d->q_ptr = this;
    }

    LabelPropertyMapper::~LabelPropertyMapper() = default;

    dspx::SelectionModel *LabelPropertyMapper::selectionModel() const {
        Q_D(const LabelPropertyMapper);
        return d->selectionModel;
    }

    void LabelPropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(LabelPropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant LabelPropertyMapper::pos() const {
        Q_D(const LabelPropertyMapper);
        return d->value<LabelPropertyMapperPrivate::PosProperty>();
    }

    void LabelPropertyMapper::setPos(const QVariant &pos) {
        Q_D(LabelPropertyMapper);
        if (!d->labelSelectionModel) {
            return;
        }
        const int value = pos.toInt();
        const auto labels = d->labelSelectionModel->selectedItems();
        for (auto *label : labels) {
            label->setPos(value);
        }
    }
    QVariant LabelPropertyMapper::text() const {
        Q_D(const LabelPropertyMapper);
        return d->value<LabelPropertyMapperPrivate::TextProperty>();
    }

    void LabelPropertyMapper::setText(const QVariant &text) {
        Q_D(LabelPropertyMapper);
        if (!d->labelSelectionModel) {
            return;
        }
        const QString value = text.toString();
        const auto labels = d->labelSelectionModel->selectedItems();
        for (auto *label : labels) {
            label->setText(value);
        }
    }

    void LabelPropertyMapperPrivate::setSelectionModel(dspx::SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        refreshCache();
    }

    void LabelPropertyMapperPrivate::attachSelectionModel() {
        Q_Q(LabelPropertyMapper);
        if (!selectionModel) {
            return;
        }
        labelSelectionModel = selectionModel->labelSelectionModel();
        if (!labelSelectionModel) {
            return;
        }
        QObject::connect(labelSelectionModel, &dspx::LabelSelectionModel::itemSelected, q, [this](dspx::Label *label, bool selected) {
            handleItemSelected(label, selected);
        });
        const auto existing = labelSelectionModel->selectedItems();
        for (auto *label : existing) {
            addItem(label);
        }
        refreshCache();
    }

    void LabelPropertyMapperPrivate::detachSelectionModel() {
        if (labelSelectionModel) {
            QObject::disconnect(labelSelectionModel, nullptr, q_ptr, nullptr);
        }
        clear();
        labelSelectionModel = nullptr;
        selectionModel = nullptr;
    }

}

#include "moc_LabelPropertyMapper.cpp"
