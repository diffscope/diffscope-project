#include "LabelPropertyMapper.h"
#include "LabelPropertyMapper_p.h"

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/SelectionModel.h>

namespace Core {
    LabelPropertyMapper::LabelPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new LabelPropertyMapperPrivate(this)) {
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
        return d->cachedPos;
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
        return d->cachedText;
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
    LabelPropertyMapperPrivate::LabelPropertyMapperPrivate(LabelPropertyMapper *q)
        : q_ptr(q) {
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
            addLabel(label);
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

    void LabelPropertyMapperPrivate::handleItemSelected(dspx::Label *label, bool selected) {
        if (selected) {
            if (!labelToPos.contains(label)) {
                addLabel(label);
            }
        } else {
            if (labelToPos.contains(label)) {
                removeLabel(label);
            }
        }
        refreshCache();
    }
    void LabelPropertyMapperPrivate::addLabel(dspx::Label *label) {
        Q_Q(LabelPropertyMapper);
        updatePos(label, label->pos());
        updateText(label, label->text());

        QObject::connect(label, &dspx::Label::posChanged, q, [this, label](int pos) {
            updatePos(label, pos);
        });
        QObject::connect(label, &dspx::Label::textChanged, q, [this, label](const QString &text) {
            updateText(label, text);
        });
        QObject::connect(label, &QObject::destroyed, q, [this, label] {
            removeLabel(label);
            refreshCache();
        });
    }
    void LabelPropertyMapperPrivate::removeLabel(dspx::Label *label) {
        QObject::disconnect(label, nullptr, q_ptr, nullptr);

        if (labelToPos.contains(label)) {
            const int oldPos = labelToPos.value(label);
            posToLabels[oldPos].remove(label);
            if (posToLabels[oldPos].isEmpty()) {
                posToLabels.remove(oldPos);
            }
            labelToPos.remove(label);
        }

        if (labelToText.contains(label)) {
            const QString oldText = labelToText.value(label);
            textToLabels[oldText].remove(label);
            if (textToLabels[oldText].isEmpty()) {
                textToLabels.remove(oldText);
            }
            labelToText.remove(label);
        }
    }
    void LabelPropertyMapperPrivate::clear() {
        for (auto *label : labelToPos.keys()) {
            QObject::disconnect(label, nullptr, q_ptr, nullptr);
        }
        posToLabels.clear();
        textToLabels.clear();
        labelToPos.clear();
        labelToText.clear();
        cachedPos.clear();
        cachedText.clear();
    }
    void LabelPropertyMapperPrivate::updatePos(dspx::Label *label, int pos) {
        if (labelToPos.contains(label)) {
            const int oldPos = labelToPos.value(label);
            if (oldPos == pos) {
                return;
            }
            posToLabels[oldPos].remove(label);
            if (posToLabels[oldPos].isEmpty()) {
                posToLabels.remove(oldPos);
            }
        }
        labelToPos.insert(label, pos);
        posToLabels[pos].insert(label);
        refreshCache();
    }
    void LabelPropertyMapperPrivate::updateText(dspx::Label *label, const QString &text) {
        if (labelToText.contains(label)) {
            const QString oldText = labelToText.value(label);
            if (oldText == text) {
                return;
            }
            textToLabels[oldText].remove(label);
            if (textToLabels[oldText].isEmpty()) {
                textToLabels.remove(oldText);
            }
        }
        labelToText.insert(label, text);
        textToLabels[text].insert(label);
        refreshCache();
    }
    QVariant LabelPropertyMapperPrivate::unifiedPos() const {
        const int count = labelToPos.size();
        if (count == 0 || posToLabels.size() != 1) {
            return {};
        }
        const auto it = posToLabels.constBegin();
        if (it.value().size() != count) {
            return {};
        }
        return it.key();
    }
    QVariant LabelPropertyMapperPrivate::unifiedText() const {
        const int count = labelToText.size();
        if (count == 0 || textToLabels.size() != 1) {
            return {};
        }
        const auto it = textToLabels.constBegin();
        if (it.value().size() != count) {
            return {};
        }
        return it.key();
    }
    void LabelPropertyMapperPrivate::refreshCache() {
        Q_Q(LabelPropertyMapper);
        const QVariant newPos = unifiedPos();
        const QVariant newText = unifiedText();
        if (newPos != cachedPos) {
            cachedPos = newPos;
            Q_EMIT q->posChanged();
        }
        if (newText != cachedText) {
            cachedText = newText;
            Q_EMIT q->textChanged();
        }
    }
}

#include "moc_LabelPropertyMapper.cpp"
