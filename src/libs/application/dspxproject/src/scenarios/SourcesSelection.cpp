// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SourcesSelection.h"

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>

#include <dspxproject/SingerComparison.h>

namespace DspxProject {

    namespace {

        QList<double> logicalRatios(const QList<double> &storedRatios, int singerCount) {
            if (singerCount <= 0)
                return {};

            QList<double> result;
            result.reserve(singerCount);
            double sum = 0.0;
            for (int index = 0; index < singerCount - 1; ++index) {
                const double ratio = index < storedRatios.size() ? storedRatios.at(index) : 0.0;
                result.append(ratio);
                sum += ratio;
            }
            result.append(std::max(0.0, 1.0 - sum));
            return result;
        }

        QList<double> storedRatios(QList<double> logicalRatios) {
            if (logicalRatios.size() <= 1)
                return {};
            logicalRatios.removeLast();
            return logicalRatios;
        }

        QList<double> resizedRatios(const QList<double> &ratios, int oldSingerCount, int newSingerCount) {
            if (oldSingerCount == newSingerCount)
                return ratios;
            if (newSingerCount <= 0)
                return {};

            auto weights = logicalRatios(ratios, oldSingerCount);
            if (newSingerCount < oldSingerCount) {
                weights.resize(newSingerCount);
                const double sum = std::accumulate(weights.cbegin(), weights.cend(), 0.0);
                if (sum > 0.0) {
                    for (double &weight : weights)
                        weight /= sum;
                } else {
                    const double weight = 1.0 / static_cast<double>(newSingerCount);
                    std::fill(weights.begin(), weights.end(), weight);
                }
            } else {
                const double oldScale = static_cast<double>(oldSingerCount) / static_cast<double>(newSingerCount);
                for (double &weight : weights)
                    weight *= oldScale;
                const double newSingerRatio = 1.0 / static_cast<double>(newSingerCount);
                while (weights.size() < newSingerCount)
                    weights.append(newSingerRatio);
            }
            return storedRatios(std::move(weights));
        }

    }

    std::optional<opendspx::Sources> SourcesSelection::toOpenDSPX() const {
        if (architectureId.isEmpty() || singers.isEmpty())
            return std::nullopt;
        return opendspx::Sources{
            .category = architectureId.toStdString(),
            .singers = std::vector<opendspx::SingerRef>(singers.cbegin(), singers.cend()),
        };
    }

    bool SourcesSelection::applyTo(dspx::SingingClip *clip) const {
        if (!clip)
            return false;

        const auto data = toOpenDSPX();
        if (!data) {
            if (auto *sources = clip->sources()) {
                clip->model()->destroyItem(sources);
                return true;
            }
            return false;
        }

        auto *sources = clip->sources();
        if (sources && sources->category() == architectureId
            && std::ranges::equal(sources->singers()->toOpenDSPX(), data->singers, singerDefinitionEquals)) {
            return false;
        }
        if (!sources) {
            sources = clip->model()->createSources();
            clip->setSources(sources);
        }

        const int oldSingerCount = sources->singers()->size();
        const int newSingerCount = static_cast<int>(data->singers.size());
        if (oldSingerCount != newSingerCount) {
            for (auto *anchor : sources->dynamicMixingAnchors()->asRange())
                anchor->setRatio(resizedRatios(anchor->ratio(), oldSingerCount, newSingerCount));
        }

        sources->setCategory(architectureId);
        auto *singerList = sources->singers();
        const auto oldSingers = singerList->items();
        singerList->fromOpenDSPX(data->singers);
        for (auto *oldSinger : oldSingers)
            sources->model()->destroyItem(oldSinger);
        return true;
    }

}
