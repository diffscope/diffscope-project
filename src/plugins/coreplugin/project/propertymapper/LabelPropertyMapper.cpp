#include "LabelPropertyMapper.h"
#include "LabelPropertyMapper_p.h"

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/SelectionModel.h>

namespace Core {
    LabelPropertyMapper::LabelPropertyMapper(QObject *parent) : QObject(parent), d_ptr(new LabelPropertyMapperPrivate(this)) {
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
        if (!d->selectionModel) {
            return;
        }
        if (!pos.canConvert<int>()) {
            return;
        }
        auto *model = d->labelSelectionModel;
        if (!model) {
            return;
        }
        const int value = pos.toInt();
        const auto labels = model->selectedItems();
        for (auto *label : labels) {
            if (label) {
                label->setPos(value);
            }
        }
    }

    QVariant LabelPropertyMapper::text() const {
        Q_D(const LabelPropertyMapper);
        return d->cachedText;
    }

    void LabelPropertyMapper::setText(const QVariant &text) {
        Q_D(LabelPropertyMapper);
        if (!d->selectionModel) {
            return;
        }
        if (!text.canConvert<QString>()) {
            return;
        }
        auto *model = d->labelSelectionModel;
        if (!model) {
            return;
        }
        const QString value = text.toString();
        const auto labels = model->selectedItems();
        for (auto *label : labels) {
            if (label) {
                label->setText(value);
            }
        }
    }

    LabelPropertyMapperPrivate::LabelPropertyMapperPrivate(LabelPropertyMapper *q) : q_ptr(q) {
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
        if (!label) {
            return;
        }
        if (selected) {
            if (!labelConnections.contains(label)) {
                addLabel(label);
            }
        } else {
            if (labelConnections.contains(label)) {
                removeLabel(label);
            }
        }
        refreshCache();
    }

    void LabelPropertyMapperPrivate::addLabel(dspx::Label *label) {
        Q_Q(LabelPropertyMapper);
        if (!label) {
            return;
        }
        if (labelConnections.contains(label)) {
            return;
        }

        updatePos(label, label->pos());
        updateText(label, label->text());

        LabelConnections connections;
        connections.posChanged = QObject::connect(label, &dspx::Label::posChanged, q, [this, label](int pos) {
            updatePos(label, pos);
        });
        connections.textChanged = QObject::connect(label, &dspx::Label::textChanged, q, [this, label](const QString &text) {
            updateText(label, text);
        });
        connections.destroyed = QObject::connect(label, &QObject::destroyed, q, [this, label] {
            removeLabel(label);
            refreshCache();
        });

        labelConnections.insert(label, connections);
    }

    void LabelPropertyMapperPrivate::removeLabel(dspx::Label *label) {
        if (!label) {
            return;
        }
        if (auto it = labelConnections.find(label); it != labelConnections.end()) {
            QObject::disconnect(it->posChanged);
            QObject::disconnect(it->textChanged);
            QObject::disconnect(it->destroyed);
            labelConnections.erase(it);
        }

        if (auto it = labelToPos.find(label); it != labelToPos.end()) {
            int oldPos = it.value();
            if (auto setIt = posToLabels.find(oldPos); setIt != posToLabels.end()) {
                setIt->remove(label);
                if (setIt->isEmpty()) {
                    posToLabels.erase(setIt);
                }
            }
            labelToPos.erase(it);
        }

        if (auto it = labelToText.find(label); it != labelToText.end()) {
            const QString oldText = it.value();
            if (auto setIt = textToLabels.find(oldText); setIt != textToLabels.end()) {
                setIt->remove(label);
                if (setIt->isEmpty()) {
                    textToLabels.erase(setIt);
                }
            }
            labelToText.erase(it);
        }
    }

    void LabelPropertyMapperPrivate::clear() {
        for (auto it = labelConnections.begin(); it != labelConnections.end(); ++it) {
            QObject::disconnect(it->posChanged);
            QObject::disconnect(it->textChanged);
            QObject::disconnect(it->destroyed);
        }
        labelConnections.clear();
        posToLabels.clear();
        textToLabels.clear();
        labelToPos.clear();
        labelToText.clear();
        cachedPos.clear();
        cachedText.clear();
    }

    void LabelPropertyMapperPrivate::updatePos(dspx::Label *label, int pos) {
        if (!label) {
            return;
        }
        if (auto it = labelToPos.find(label); it != labelToPos.end()) {
            const int oldPos = it.value();
            if (oldPos == pos) {
                return;
            }
            if (auto setIt = posToLabels.find(oldPos); setIt != posToLabels.end()) {
                setIt->remove(label);
                if (setIt->isEmpty()) {
                    posToLabels.erase(setIt);
                }
            }
        }
        labelToPos.insert(label, pos);
        posToLabels[pos].insert(label);
        refreshCache();
    }

    void LabelPropertyMapperPrivate::updateText(dspx::Label *label, const QString &text) {
        if (!label) {
            return;
        }
        if (auto it = labelToText.find(label); it != labelToText.end()) {
            const QString oldText = it.value();
            if (oldText == text) {
                return;
            }
            if (auto setIt = textToLabels.find(oldText); setIt != textToLabels.end()) {
                setIt->remove(label);
                if (setIt->isEmpty()) {
                    textToLabels.erase(setIt);
                }
            }
        }
        labelToText.insert(label, text);
        textToLabels[text].insert(label);
        refreshCache();
    }

    QVariant LabelPropertyMapperPrivate::unifiedPos() const {
        const int count = labelConnections.size();
        if (count == 0) {
            return {};
        }
        if (posToLabels.size() != 1) {
            return {};
        }
        const auto it = posToLabels.constBegin();
        if (it.value().size() != count) {
            return {};
        }
        return it.key();
    }

    QVariant LabelPropertyMapperPrivate::unifiedText() const {
        const int count = labelConnections.size();
        if (count == 0) {
            return {};
        }
        if (textToLabels.size() != 1) {
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
