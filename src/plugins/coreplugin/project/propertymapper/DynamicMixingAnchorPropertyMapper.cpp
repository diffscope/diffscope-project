#include "DynamicMixingAnchorPropertyMapper.h"
#include "DynamicMixingAnchorPropertyMapper_p.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelSelectionModel/DynamicMixingAnchorSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace Core {

    void DynamicMixingAnchorPropertyMapperPrivate::disconnectAll() {
        for (const auto &connection : std::as_const(connections))
            QObject::disconnect(connection);
        connections.clear();
        sequence = nullptr;
    }

    QList<dspx::DynamicMixingAnchor *> DynamicMixingAnchorPropertyMapperPrivate::selectedItems() const {
        if (!selectionModel
            || selectionModel->selectionType() != dspx::SelectionModel::ST_DynamicMixingAnchor) {
            return {};
        }
        return selectionModel->dynamicMixingAnchorSelectionModel()->selectedItems();
    }

    QList<double> DynamicMixingAnchorPropertyMapperPrivate::effectiveRatios(
        const dspx::DynamicMixingAnchor *item) const {
        const int count = sequence && sequence->sources() ? sequence->sources()->singers()->size() : 0;
        if (!item || count <= 0)
            return {};
        QList<double> result;
        result.reserve(count);
        const auto stored = item->ratio();
        double sum = 0.0;
        for (int index = 0; index < count - 1; ++index) {
            const double value = index < stored.size() ? stored.at(index) : 0.0;
            result.append(value);
            sum += value;
        }
        result.append(std::max(0.0, 1.0 - sum));
        return result;
    }

    void DynamicMixingAnchorPropertyMapperPrivate::notifyAll() {
        Q_Q(DynamicMixingAnchorPropertyMapper);
        Q_EMIT q->positionChanged();
        Q_EMIT q->singersChanged();
        Q_EMIT q->ratiosChanged();
    }

    void DynamicMixingAnchorPropertyMapperPrivate::refreshConnections() {
        Q_Q(DynamicMixingAnchorPropertyMapper);
        disconnectAll();
        if (!selectionModel) {
            notifyAll();
            return;
        }

        auto *dynamicSelection = selectionModel->dynamicMixingAnchorSelectionModel();
        sequence = dynamicSelection->dynamicMixingAnchorSequenceWithSelectedItems();
        connections.append(QObject::connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged,
                                            q, [this] { refreshConnections(); }));
        connections.append(QObject::connect(dynamicSelection,
                                            &dspx::DynamicMixingAnchorSelectionModel::selectedItemsChanged,
                                            q, [this] { refreshConnections(); }));
        connections.append(QObject::connect(dynamicSelection,
                                            &dspx::DynamicMixingAnchorSelectionModel::dynamicMixingAnchorSequenceWithSelectedItemsChanged,
                                            q, [this] { refreshConnections(); }));
        for (auto *item : selectedItems()) {
            connections.append(QObject::connect(item, &dspx::DynamicMixingAnchor::positionChanged,
                                                q, &DynamicMixingAnchorPropertyMapper::positionChanged));
            connections.append(QObject::connect(item, &dspx::DynamicMixingAnchor::ratioChanged,
                                                q, &DynamicMixingAnchorPropertyMapper::ratiosChanged));
        }
        if (sequence && sequence->sources()) {
            auto *singerList = sequence->sources()->singers();
            connections.append(QObject::connect(singerList, &dspx::SingerList::itemsChanged,
                                                q, [this] {
                Q_Q(DynamicMixingAnchorPropertyMapper);
                Q_EMIT q->singersChanged();
                Q_EMIT q->ratiosChanged();
            }));
        }
        notifyAll();
    }

    DynamicMixingAnchorPropertyMapper::DynamicMixingAnchorPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new DynamicMixingAnchorPropertyMapperPrivate) {
        Q_D(DynamicMixingAnchorPropertyMapper);
        d->q_ptr = this;
    }

    DynamicMixingAnchorPropertyMapper::~DynamicMixingAnchorPropertyMapper() = default;

    dspx::SelectionModel *DynamicMixingAnchorPropertyMapper::selectionModel() const {
        Q_D(const DynamicMixingAnchorPropertyMapper);
        return d->selectionModel;
    }

    void DynamicMixingAnchorPropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(DynamicMixingAnchorPropertyMapper);
        if (d->selectionModel == selectionModel)
            return;
        d->disconnectAll();
        d->selectionModel = selectionModel;
        d->refreshConnections();
        Q_EMIT selectionModelChanged();
    }

    QVariant DynamicMixingAnchorPropertyMapper::position() const {
        Q_D(const DynamicMixingAnchorPropertyMapper);
        const auto items = d->selectedItems();
        if (items.isEmpty())
            return {};
        const int value = items.first()->position();
        return std::ranges::all_of(items, [value](const auto *item) {
            return item->position() == value;
        }) ? QVariant(value) : QVariant{};
    }

    int DynamicMixingAnchorPropertyMapper::voiceCount() const {
        Q_D(const DynamicMixingAnchorPropertyMapper);
        return d->sequence && d->sequence->sources()
                   ? d->sequence->sources()->singers()->size()
                   : 0;
    }

    QObjectList DynamicMixingAnchorPropertyMapper::singers() const {
        Q_D(const DynamicMixingAnchorPropertyMapper);
        QObjectList result;
        if (!d->sequence || !d->sequence->sources())
            return result;
        for (auto *singer : d->sequence->sources()->singers()->items())
            result.append(singer);
        return result;
    }

    QVariant DynamicMixingAnchorPropertyMapper::ratios() const {
        Q_D(const DynamicMixingAnchorPropertyMapper);
        const auto items = d->selectedItems();
        if (items.isEmpty())
            return {};
        const auto value = d->effectiveRatios(items.first());
        if (value.isEmpty() || !std::ranges::all_of(items, [d, &value](const auto *item) {
                return d->effectiveRatios(item) == value;
            })) {
            return {};
        }
        QVariantList result;
        result.reserve(value.size());
        for (const double ratio : value)
            result.append(ratio);
        return result;
    }

    QVariant DynamicMixingAnchorPropertyMapper::ratioAt(int singerIndex) const {
        Q_D(const DynamicMixingAnchorPropertyMapper);
        const auto items = d->selectedItems();
        if (items.isEmpty() || singerIndex < 0 || singerIndex >= voiceCount())
            return {};
        const auto first = d->effectiveRatios(items.first());
        if (singerIndex >= first.size())
            return {};
        const double value = first.at(singerIndex);
        for (auto *item : items) {
            const auto ratios = d->effectiveRatios(item);
            if (singerIndex >= ratios.size() || ratios.at(singerIndex) != value)
                return {};
        }
        return value;
    }

    QVariant DynamicMixingAnchorPropertyMapper::maximumRatioAt(int singerIndex) const {
        Q_D(const DynamicMixingAnchorPropertyMapper);
        const auto items = d->selectedItems();
        const int count = voiceCount();
        if (items.isEmpty() || count < 1 || singerIndex < 0 || singerIndex >= count)
            return {};
        if (count == 1)
            return 1.0;
        const int adjacent = singerIndex + 1 < count ? singerIndex + 1 : singerIndex - 1;
        double maximum = 1.0;
        for (auto *item : items) {
            const auto values = d->effectiveRatios(item);
            maximum = std::min(maximum, values.at(singerIndex) + values.at(adjacent));
        }
        return maximum;
    }

    bool DynamicMixingAnchorPropertyMapper::setSingerRatio(int singerIndex, double ratio) {
        Q_D(DynamicMixingAnchorPropertyMapper);
        const auto items = d->selectedItems();
        const int count = voiceCount();
        if (items.isEmpty() || count < 2 || singerIndex < 0 || singerIndex >= count
            || !std::isfinite(ratio)) {
            return false;
        }
        const int leftIndex = singerIndex + 1 < count ? singerIndex : singerIndex - 1;
        for (auto *item : items) {
            auto values = d->effectiveRatios(item);
            const double pairSum = values.at(leftIndex) + values.at(leftIndex + 1);
            const double targetSinger = std::clamp(ratio, 0.0, pairSum);
            if (singerIndex == leftIndex) {
                values[leftIndex] = targetSinger;
                values[leftIndex + 1] = pairSum - targetSinger;
            } else {
                values[leftIndex] = pairSum - targetSinger;
                values[leftIndex + 1] = targetSinger;
            }
            values.removeLast();
            item->setRatio(values);
        }
        return true;
    }

    bool DynamicMixingAnchorPropertyMapper::setAdjacentRatios(int leftSingerIndex,
                                                               double leftRatio) {
        Q_D(DynamicMixingAnchorPropertyMapper);
        const auto items = d->selectedItems();
        const int count = voiceCount();
        if (items.isEmpty() || leftSingerIndex < 0 || leftSingerIndex + 1 >= count
            || !std::isfinite(leftRatio)) {
            return false;
        }
        for (auto *item : items) {
            auto values = d->effectiveRatios(item);
            const double pairSum = values.at(leftSingerIndex) + values.at(leftSingerIndex + 1);
            values[leftSingerIndex] = std::clamp(leftRatio, 0.0, pairSum);
            values[leftSingerIndex + 1] = pairSum - values.at(leftSingerIndex);
            values.removeLast();
            item->setRatio(values);
        }
        return true;
    }

}

#include "moc_DynamicMixingAnchorPropertyMapper.cpp"
