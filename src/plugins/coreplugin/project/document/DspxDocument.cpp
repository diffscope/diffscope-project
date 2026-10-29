// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DspxDocument.h"
#include "DspxDocument_p.h"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <QGuiApplication>
#include <QHash>
#include <QLoggingCategory>
#include <QMap>
#include <QPointer>
#include <QQuickWindow>
#include <QSet>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/clip.h>
#include <opendspx/mixedsinger.h>
#include <opendspx/singlesinger.h>
#include <opendspx/sources.h>
#include <opendspx/track.h>

#include <SVSCraftQuick/MessageBox.h>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/DynamicMixingAnchorSelectionModel.h>
#include <dspxmodelSelectionModel/KeySignatureSelectionModel.h>
#include <dspxmodelSelectionModel/LabelSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelSelectionModel/TempoSelectionModel.h>
#include <dspxmodelSelectionModel/TrackSelectionModel.h>

#include <coreplugin/ArchitectureInfo.h>
#include <coreplugin/DspxClipboard.h>
#include <coreplugin/FreeParameterSelectionModel.h>
#include <coreplugin/ParameterResamplingScenario.h>
#include <coreplugin/internal/FreeParameterResampling.h>

#include <stdcorelib/support/json.h>
#include <transactional/TransactionController.h>
#include <transactional/TransactionalStrategy.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcDspxDocument, "diffscope.core.dspxdocument")

    static const char *selectionTypeName(dspx::SelectionModel::SelectionType type) {
        switch (type) {
            case dspx::SelectionModel::ST_None:
                return "None";
            case dspx::SelectionModel::ST_AnchorNode:
                return "AnchorNode";
            case dspx::SelectionModel::ST_Clip:
                return "Clip";
            case dspx::SelectionModel::ST_Label:
                return "Label";
            case dspx::SelectionModel::ST_Note:
                return "Note";
            case dspx::SelectionModel::ST_Tempo:
                return "Tempo";
            case dspx::SelectionModel::ST_Track:
                return "Track";
            case dspx::SelectionModel::ST_KeySignature:
                return "KeySignature";
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                return "DynamicMixingAnchor";
        }
        return "Unknown";
    }

    namespace {

        struct ClipboardHandler {
            DspxClipboardData::Type type;
            std::optional<DspxClipboardData> (DspxDocumentPrivate::*copy)(int) const;
            bool (DspxDocumentPrivate::*paste)(const DspxClipboardData &, DspxDocumentPrivate::PasteContext &);
            const char *transactionName;
        };

        const ClipboardHandler *clipboardHandler(DspxClipboardData::Type type) {
            static const auto handlers = std::to_array<ClipboardHandler>({
                {DspxClipboardData::Tempo, &DspxDocumentPrivate::buildTempoClipboardData, &DspxDocumentPrivate::pasteTempos, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting tempo")},
                {DspxClipboardData::Label, &DspxDocumentPrivate::buildLabelClipboardData, &DspxDocumentPrivate::pasteLabels, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting label")},
                {DspxClipboardData::KeySignature, &DspxDocumentPrivate::buildKeySignatureClipboardData, &DspxDocumentPrivate::pasteKeySignatures, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting key signature")},
                {DspxClipboardData::Track, &DspxDocumentPrivate::buildTrackClipboardData, &DspxDocumentPrivate::pasteTracks, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting track")},
                {DspxClipboardData::Clip, &DspxDocumentPrivate::buildClipClipboardData, &DspxDocumentPrivate::pasteClips, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting clip")},
                {DspxClipboardData::Note, &DspxDocumentPrivate::buildNoteClipboardData, &DspxDocumentPrivate::pasteNotes, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting note")},
                {DspxClipboardData::DynamicMixingAnchor, &DspxDocumentPrivate::buildDynamicMixingAnchorClipboardData, &DspxDocumentPrivate::pasteDynamicMixingAnchors, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting voice blending anchor")},
                {DspxClipboardData::AnchorNode, &DspxDocumentPrivate::buildAnchorNodeClipboardData, &DspxDocumentPrivate::pasteAnchorNodes, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting anchor node")},
                {DspxClipboardData::FreeParameter, &DspxDocumentPrivate::buildFreeParameterClipboardData, &DspxDocumentPrivate::pasteFreeParameter, QT_TRANSLATE_NOOP("Core::DspxDocument", "Pasting parameter data")},
            });
            const auto it = std::ranges::find(handlers, type, &ClipboardHandler::type);
            return it == handlers.end() ? nullptr : &*it;
        }

        template<typename T>
        int itemPosition(const T &item) {
            if constexpr (requires { item.pos; })
                return item.pos;
            else
                return item.x;
        }

        int itemPosition(const stdc::json::Value &item) {
            return static_cast<int>(item["pos"].toInt());
        }

        template<typename T>
        void setItemPosition(T &item, int position) {
            if constexpr (requires { item.pos; })
                item.pos = position;
            else
                item.x = position;
        }

        void setItemPosition(stdc::json::Value &item, int position) {
            auto object = item.toObject();
            object["pos"] = position;
            item = std::move(object);
        }

        template<typename T>
        bool itemPrecedes(const T &left, const T &right) {
            if (itemPosition(left) != itemPosition(right))
                return itemPosition(left) < itemPosition(right);
            if constexpr (std::is_same_v<T, opendspx::Note>) {
                if (left.keyNum != right.keyNum)
                    return left.keyNum < right.keyNum;
                return left.length < right.length;
            }
            return false;
        }

        template<typename Item, typename Setter>
        std::optional<DspxClipboardData> buildSequenceData(const QList<Item *> &items, int playhead, Setter setter) {
            if (items.isEmpty())
                return std::nullopt;
            using Data = decltype(items.first()->toOpenDSPX());
            QList<Data> values;
            values.reserve(items.size());
            for (const auto *item : items)
                values.append(item->toOpenDSPX());
            std::stable_sort(values.begin(), values.end(), itemPrecedes<Data>);
            const int absolute = itemPosition(values.first());
            for (auto &value : values)
                setItemPosition(value, itemPosition(value) - absolute);
            DspxClipboardData data;
            (data.*setter)(values);
            data.setAbsolute(absolute);
            data.setPlayhead(playhead - absolute);
            return data;
        }

        template<typename Data, typename Sequence, typename Factory>
        bool pasteSequenceData(QList<Data> values, Sequence *sequence, dspx::Model *model,
                               DspxDocumentPrivate::PasteContext &context, Factory createItem, bool replaceOverlaps) {
            if (!sequence || values.isEmpty())
                return false;
            int minimumPosition = std::numeric_limits<int>::max();
            for (const auto &value : std::as_const(values))
                minimumPosition = std::min(minimumPosition, itemPosition(value));
            const qint64 offset = std::max<qint64>(context.position, -static_cast<qint64>(minimumPosition));
            for (auto &value : values) {
                const qint64 position = itemPosition(value) + offset;
                if (position > std::numeric_limits<int>::max())
                    return false;
                setItemPosition(value, static_cast<int>(position));
            }
            std::stable_sort(values.begin(), values.end(), itemPrecedes<Data>);
            bool inserted = false;
            for (qsizetype index = 0; index < values.size(); ++index) {
                const auto &value = values.at(index);
                if (replaceOverlaps) {
                    if (index + 1 < values.size() && itemPosition(values.at(index + 1)) == itemPosition(value))
                        continue;
                    for (auto *overlap : sequence->slice(itemPosition(value), 1)) {
                        sequence->removeItem(overlap);
                        model->destroyItem(overlap);
                    }
                }
                auto *item = createItem();
                item->fromOpenDSPX(value);
                if (!sequence->insertItem(item)) {
                    model->destroyItem(item);
                    return false;
                }
                context.items.append(item);
                inserted = true;
            }
            return inserted;
        }

        struct BounceInfo {
            dspx::SingingClip *pivotClip{};
            QList<dspx::SingingClip *> clips;
            opendspx::ClipTime bouncedClipTime;
        };

        using TrackBounceInfoMap = QHash<dspx::Track *, BounceInfo>;

        opendspx::ClipTime boundedClipTime(const dspx::SingingClip *clip) {
            return {
                .pos = clip->position(),
                .length = 0,
                .clipStart = 0,
                .clipLen = clip->clipLength(),
            };
        }

        bool clipPrecedes(const dspx::SingingClip *left, const dspx::SingingClip *right) {
            if (left->position() != right->position())
                return left->position() < right->position();
            if (left->clipLength() != right->clipLength())
                return left->clipLength() < right->clipLength();
            return std::less<const dspx::SingingClip *>()(left, right);
        }

        bool clipPreferredAsPivot(const dspx::SingingClip *candidate,
                                  const dspx::SingingClip *current,
                                  const dspx::NoteSequence *preferredNoteSequence) {
            const bool candidatePreferred = candidate->notes() == preferredNoteSequence;
            const bool currentPreferred = current->notes() == preferredNoteSequence;
            if (candidatePreferred != currentPreferred)
                return candidatePreferred;
            return clipPrecedes(candidate, current);
        }

        TrackBounceInfoMap buildTrackBounceInfoMap(const dspx::SelectionModel *selectionModel) {
            TrackBounceInfoMap result;
            if (!selectionModel || selectionModel->selectionType() != dspx::SelectionModel::ST_Clip)
                return result;

            const auto *preferredNoteSequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
            for (auto *clip : selectionModel->clipSelectionModel()->selectedItems()) {
                auto *singingClip = qobject_cast<dspx::SingingClip *>(clip);
                if (!singingClip || !singingClip->clipSequence())
                    continue;

                auto *track = singingClip->clipSequence()->track();
                auto &info = result[track];
                const auto clipTime = boundedClipTime(singingClip);
                if (!info.pivotClip) {
                    info.pivotClip = singingClip;
                    info.bouncedClipTime = clipTime;
                } else {
                    if (clipPreferredAsPivot(singingClip, info.pivotClip, preferredNoteSequence))
                        info.pivotClip = singingClip;
                    const int left = std::min(info.bouncedClipTime.pos, clipTime.pos);
                    const int right = std::max(info.bouncedClipTime.pos + info.bouncedClipTime.clipLen,
                                               clipTime.pos + clipTime.clipLen);
                    info.bouncedClipTime.pos = left;
                    info.bouncedClipTime.clipLen = right - left;
                }
                info.clips.append(singingClip);
            }

            for (auto it = result.begin(); it != result.end(); ++it)
                std::sort(it->clips.begin(), it->clips.end(), clipPrecedes);
            return result;
        }

        bool singersIdentical(const std::vector<opendspx::SingerRef> &left,
                              const std::vector<opendspx::SingerRef> &right);

        bool workspacesIdentical(const opendspx::Workspace &left, const opendspx::Workspace &right) {
            if (left.size() != right.size())
                return false;
            auto leftIt = left.cbegin();
            auto rightIt = right.cbegin();
            for (; leftIt != left.cend(); ++leftIt, ++rightIt) {
                if (leftIt->first != rightIt->first || leftIt->second != rightIt->second)
                    return false;
            }
            return true;
        }

        bool singerIdentical(const opendspx::SingerRef &left, const opendspx::SingerRef &right) {
            if (!left || !right)
                return left == right;
            if (left->type != right->type || left->extra != right->extra ||
                !workspacesIdentical(left->workspace, right->workspace)) {
                return false;
            }

            switch (left->type) {
                case opendspx::Singer::Type::Single: {
                    const auto &leftSingle = static_cast<const opendspx::SingleSinger &>(*left);
                    const auto &rightSingle = static_cast<const opendspx::SingleSinger &>(*right);
                    return leftSingle.id == rightSingle.id;
                }
                case opendspx::Singer::Type::Mixed: {
                    const auto &leftMixed = static_cast<const opendspx::MixedSinger &>(*left);
                    const auto &rightMixed = static_cast<const opendspx::MixedSinger &>(*right);
                    return leftMixed.ratio.size() == rightMixed.ratio.size() &&
                           std::equal(leftMixed.ratio.cbegin(), leftMixed.ratio.cend(),
                                      rightMixed.ratio.cbegin()) &&
                           singersIdentical(leftMixed.singers, rightMixed.singers);
                }
            }
            return false;
        }

        bool singersIdentical(const std::vector<opendspx::SingerRef> &left,
                              const std::vector<opendspx::SingerRef> &right) {
            if (left.size() != right.size())
                return false;
            for (std::size_t i = 0; i < left.size(); ++i) {
                if (!singerIdentical(left[i], right[i]))
                    return false;
            }
            return true;
        }

        std::vector<opendspx::SingerRef> clipSingers(const dspx::SingingClip *clip) {
            auto *sources = clip ? clip->sources() : nullptr;
            return sources ? sources->singers()->toOpenDSPX() : std::vector<opendspx::SingerRef> {};
        }

        bool bounceInfoSingerLayoutIdentical(const BounceInfo &info) {
            if (!info.pivotClip || info.clips.isEmpty())
                return false;
            const auto pivotSingers = clipSingers(info.pivotClip);
            return std::all_of(info.clips.cbegin(), info.clips.cend(), [&](const dspx::SingingClip *clip) {
                return singersIdentical(pivotSingers, clipSingers(clip));
            });
        }

        bool segmentIsClipped(int left, int right, int clipStart, int clipEnd) {
            if (clipEnd <= clipStart)
                return false;
            return (left < clipStart && right > clipStart) ||
                   (left < clipEnd && right >= clipEnd);
        }

        bool occupyPoint(QHash<QString, QSet<int>> &occupied, const QString &parameterId, int position) {
            auto &positions = occupied[parameterId];
            if (positions.contains(position))
                return false;
            positions.insert(position);
            return true;
        }

        struct ParameterPointOccupancy {
            QHash<QString, QSet<int>> freeTransform;
            QHash<QString, QSet<int>> freeEdited;
            QHash<QString, QSet<int>> anchorTransform;
            QHash<QString, QSet<int>> anchorEdited;
        };

        bool anchorSequenceCanSafelyBounce(const dspx::AnchorNodeSequence *sequence,
                                           const QString &parameterId,
                                           int clipStart,
                                           int clipEnd,
                                           int deltaPosition,
                                           QHash<QString, QSet<int>> &occupied) {
            const dspx::AnchorNode *previous = nullptr;
            for (auto *node : sequence->sliceEffective(clipStart, clipEnd - clipStart)) {
                if (previous && previous->interpolationMode() != dspx::AnchorNode::None &&
                    segmentIsClipped(previous->x(), node->x(), clipStart, clipEnd)) {
                    return false;
                }
                if (node->x() >= clipStart && node->x() < clipEnd &&
                    !occupyPoint(occupied, parameterId, node->x() + deltaPosition)) {
                    return false;
                }
                previous = node;
            }
            return true;
        }

        bool freeValuesCanSafelyBounce(const dspx::FreeValueDataArray *array,
                                       const QString &parameterId,
                                       int clipStart,
                                       int clipEnd,
                                       int deltaPosition,
                                       QHash<QString, QSet<int>> &occupied) {
            const auto data = Internal::captureFreeParameter(array, clipStart, clipEnd);
            if (std::ranges::none_of(data.values, [](const auto &value) { return value.isValid(); }))
                return true;
            const int step = dspx::FreeValueDataArray::step();
            const int firstIndex = clipStart / step;
            const auto &values = data.values;
            for (int offset = 0; offset < values.size(); ++offset) {
                const int index = firstIndex + offset;
                const int position = index * step;
                if (offset + 1 < values.size() && values.at(offset).isValid() &&
                    values.at(offset + 1).isValid() &&
                    segmentIsClipped(position, position + step, clipStart, clipEnd)) {
                    return false;
                }
            }
            const auto sampled = Internal::resampleFreeParameter(data, clipStart + deltaPosition);
            for (int offset = 0; offset < sampled.values.size(); ++offset) {
                if (sampled.values.at(offset).isValid() &&
                    !occupyPoint(occupied, parameterId, (sampled.firstIndex + offset) * step)) {
                    return false;
                }
            }
            return true;
        }

        bool dynamicMixingCanSafelyBounce(const dspx::DynamicMixingAnchorSequence *sequence,
                                          int clipStart,
                                          int clipEnd,
                                          int deltaPosition,
                                          QSet<int> &occupied) {
            const dspx::DynamicMixingAnchor *previous = nullptr;
            for (auto *anchor : sequence->sliceEffective(clipStart, clipEnd - clipStart)) {
                if (previous && segmentIsClipped(previous->position(), anchor->position(), clipStart, clipEnd))
                    return false;
                if (anchor->position() >= clipStart && anchor->position() < clipEnd) {
                    const int targetPosition = anchor->position() + deltaPosition;
                    if (occupied.contains(targetPosition))
                        return false;
                    occupied.insert(targetPosition);
                }
                previous = anchor;
            }
            return true;
        }

        struct AnchorPointData {
            dspx::AnchorNode::InterpolationMode interpolationMode{dspx::AnchorNode::None};
            double value{};
        };

        struct MergedParameterData {
            QMap<int, double> freeTransform;
            QMap<int, double> freeEdited;
            QMap<int, AnchorPointData> anchorTransform;
            QMap<int, AnchorPointData> anchorEdited;
        };

        using MergedParameterMap = QHash<QString, MergedParameterData>;

        void mergeFreeValues(const dspx::FreeValueDataArray *array,
                             int clipStart,
                             int clipEnd,
                             int deltaPosition,
                             QMap<int, double> &target) {
            const auto data = Internal::captureFreeParameter(array, clipStart, clipEnd);
            const auto sampled = Internal::resampleFreeParameter(data, clipStart + deltaPosition);
            for (int index = 0; index < sampled.values.size(); ++index) {
                if (sampled.values.at(index).isValid()) {
                    target.insert(sampled.firstIndex + index,
                                  ParameterInfo::fromDspxModelValue(sampled.values.at(index).toInt()));
                }
            }
        }

        void mergeAnchorNodes(const dspx::AnchorNodeSequence *sequence,
                              int clipStart,
                              int clipEnd,
                              int deltaPosition,
                              QMap<int, AnchorPointData> &target) {
            for (auto *node : sequence->asRange()) {
                if (node->x() < clipStart || node->x() >= clipEnd)
                    continue;
                target.insert(node->x() + deltaPosition, {
                    .interpolationMode = node->interpolationMode(),
                    .value = ParameterInfo::fromDspxModelValue(node->y()),
                });
            }
        }

        MergedParameterMap mergeParameterData(const BounceInfo &info) {
            MergedParameterMap result;
            for (auto *clip : info.clips) {
                const int clipStart = clip->clipStart();
                const int clipEnd = clipStart + clip->clipLength();
                const int deltaPosition = clip->start() - info.bouncedClipTime.pos;
                for (const auto &parameterId : clip->parameters()->keys()) {
                    auto *parameter = clip->parameters()->item(parameterId);
                    if (!parameter)
                        continue;
                    auto &target = result[parameterId];
                    mergeFreeValues(parameter->freeTransform(), clipStart, clipEnd, deltaPosition, target.freeTransform);
                    mergeFreeValues(parameter->freeEdited(), clipStart, clipEnd, deltaPosition, target.freeEdited);
                    mergeAnchorNodes(parameter->anchorTransform(), clipStart, clipEnd, deltaPosition, target.anchorTransform);
                    mergeAnchorNodes(parameter->anchorEdited(), clipStart, clipEnd, deltaPosition, target.anchorEdited);
                }
            }
            return result;
        }

        QMap<int, QList<double>> mergeDynamicMixingData(const BounceInfo &info) {
            QMap<int, QList<double>> result;
            for (auto *clip : info.clips) {
                auto *sources = clip->sources();
                if (!sources)
                    continue;
                const int clipStart = clip->clipStart();
                const int clipEnd = clipStart + clip->clipLength();
                const int deltaPosition = clip->start() - info.bouncedClipTime.pos;
                for (auto *anchor : sources->dynamicMixingAnchors()->asRange()) {
                    if (anchor->position() >= clipStart && anchor->position() < clipEnd)
                        result.insert(anchor->position() + deltaPosition, anchor->ratio());
                }
            }
            return result;
        }

        QList<QVariant> freeValuesFromMergedPoints(const QMap<int, double> &points) {
            if (points.isEmpty())
                return {};
            QList<QVariant> result;
            result.resize(points.lastKey() + 1);
            for (auto it = points.cbegin(); it != points.cend(); ++it)
                result[it.key()] = ParameterInfo::toDspxModelValue(it.value());
            return result;
        }

        void clearAnchorSequence(dspx::AnchorNodeSequence *sequence, dspx::Model *model) {
            while (auto *node = sequence->firstItem()) {
                sequence->removeItem(node);
                model->destroyItem(node);
            }
        }

        void applyMergedAnchorNodes(dspx::AnchorNodeSequence *sequence,
                                    const QMap<int, AnchorPointData> &points,
                                    dspx::Model *model) {
            clearAnchorSequence(sequence, model);
            for (auto it = points.cbegin(); it != points.cend(); ++it) {
                const auto &point = it.value();
                auto *node = model->createAnchorNode();
                node->setX(it.key());
                node->setY(ParameterInfo::toDspxModelValue(point.value));
                node->setInterpolationMode(point.interpolationMode);
                if (!sequence->insertItem(node))
                    model->destroyItem(node);
            }
        }

        void applyMergedParameters(dspx::SingingClip *pivotClip,
                                   const MergedParameterMap &parameters,
                                   dspx::Model *model) {
            auto *targetMap = pivotClip->parameters();
            for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
                auto *parameter = targetMap->item(it.key());
                if (!parameter) {
                    parameter = model->createParameter();
                    if (!targetMap->insertItem(it.key(), parameter)) {
                        model->destroyItem(parameter);
                        continue;
                    }
                }
                const auto &source = it.value();
                const auto freeTransform = freeValuesFromMergedPoints(source.freeTransform);
                const auto freeEdited = freeValuesFromMergedPoints(source.freeEdited);
                parameter->freeTransform()->splice(0, parameter->freeTransform()->size(), freeTransform);
                parameter->freeEdited()->splice(0, parameter->freeEdited()->size(), freeEdited);
                applyMergedAnchorNodes(parameter->anchorTransform(), source.anchorTransform, model);
                applyMergedAnchorNodes(parameter->anchorEdited(), source.anchorEdited, model);
            }
        }

        void clearDynamicMixingAnchors(dspx::DynamicMixingAnchorSequence *sequence, dspx::Model *model) {
            while (auto *anchor = sequence->firstItem()) {
                sequence->removeItem(anchor);
                model->destroyItem(anchor);
            }
        }

        void applyMergedDynamicMixing(dspx::SingingClip *pivotClip,
                                      const QMap<int, QList<double>> &anchors,
                                      dspx::Model *model) {
            auto *sources = pivotClip->sources();
            if (!sources && !anchors.isEmpty()) {
                sources = model->createSources();
                pivotClip->setSources(sources);
            }
            if (!sources)
                return;

            auto *sequence = sources->dynamicMixingAnchors();
            clearDynamicMixingAnchors(sequence, model);
            for (auto it = anchors.cbegin(); it != anchors.cend(); ++it) {
                auto *anchor = model->createDynamicMixingAnchor();
                anchor->setPosition(it.key());
                anchor->setRatio(it.value());
                if (!sequence->insertItem(anchor))
                    model->destroyItem(anchor);
            }
        }

    }

    class TransactionalModelStrategy : public TransactionalStrategy {
    public:
        explicit TransactionalModelStrategy(dspx::Document *document, QObject *parent = nullptr)
            : TransactionalStrategy(parent), m_document(document) {
        }

        void beginTransaction() override {
            if (!m_document || m_transaction)
                return;
            m_transaction.emplace(m_document->engine()->beginTransaction());
            m_document->setTransaction(&*m_transaction);
        }
        void abortTransaction() override {
            if (!m_document || !m_transaction)
                return;
            m_document->setTransaction(nullptr);
            m_transaction->rollback();
            m_transaction.reset();
        }
        void commitTransaction() override {
            if (!m_document || !m_transaction)
                return;
            m_transaction->commit();
            m_document->setTransaction(nullptr);
            m_transaction.reset();
        }
        void moveCurrentStepBy(int count) override {
            if (!m_document)
                return;
            while (count < 0) {
                m_document->engine()->undo();
                ++count;
            }
            while (count > 0) {
                m_document->engine()->redo();
                --count;
            }
        }

    private:
        dspx::Document *m_document;
        std::optional<dini::Transaction> m_transaction;

    };

    template<typename Signal>
    bool DspxDocumentPrivate::emitOnChange(bool value, bool &cache, Signal signal) {
        if (cache == value)
            return false;
        cache = value;
        Q_EMIT (q_ptr->*signal)();
        return true;
    }

    void DspxDocumentPrivate::initConnections() {
        updateAnyItemsSelected();
        updateEditScopeFocused();
        updatePasteAvailable();

        QObject::connect(selectionModel, &dspx::SelectionModel::selectedCountChanged, q_ptr, [this] {
            updateAnyItemsSelected();
        });
        QObject::connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, q_ptr, [this] {
            updateEditScopeFocused();
            updatePasteAvailable();
        });
        QObject::connect(selectionModel->noteSelectionModel(),
                         &dspx::NoteSelectionModel::noteSequenceWithSelectedItemsChanged,
                         q_ptr, [this] {
            updatePasteAvailable();
        });
        QObject::connect(selectionModel->anchorNodeSelectionModel(),
                         &dspx::AnchorNodeSelectionModel::anchorNodeSequenceWithSelectedItemsChanged,
                         q_ptr, [this] {
            updatePasteAvailable();
        });
        QObject::connect(selectionModel->dynamicMixingAnchorSelectionModel(),
                         &dspx::DynamicMixingAnchorSelectionModel::dynamicMixingAnchorSequenceWithSelectedItemsChanged,
                         q_ptr, [this] {
            updatePasteAvailable();
        });
        QObject::connect(freeParameterSelectionModel, &FreeParameterSelectionModel::hasSelectionChanged,
                         q_ptr, [this] {
            updateAnyItemsSelected();
        });
        QObject::connect(freeParameterSelectionModel, &FreeParameterSelectionModel::activeChanged,
                         q_ptr, [this] {
            updateEditScopeFocused();
            updatePasteAvailable();
        });
        QObject::connect(freeParameterSelectionModel, &FreeParameterSelectionModel::contextChanged,
                         q_ptr, [this] {
            updatePasteAvailable();
        });

        if (auto *clipboard = DspxClipboard::instance()) {
            QObject::connect(clipboard, &DspxClipboard::changed, q_ptr, [this] {
                updatePasteAvailable();
            });
        }
    }

    bool DspxDocumentPrivate::updateAnyItemsSelected() {
        const bool value = (selectionModel && selectionModel->selectedCount() > 0) ||
                           (freeParameterSelectionModel && freeParameterSelectionModel->hasSelection());
        return emitOnChange(value, anyItemsSelected, &DspxDocument::anyItemsSelectedChanged);
    }

    bool DspxDocumentPrivate::updateEditScopeFocused() {
        const bool value = (selectionModel && selectionModel->selectionType() != dspx::SelectionModel::ST_None) ||
                           (freeParameterSelectionModel && freeParameterSelectionModel->isActive());
        return emitOnChange(value, editScopeFocused, &DspxDocument::editScopeFocusedChanged);
    }

    int DspxDocumentPrivate::currentTrackIndex() const {
        if (!model || !selectionModel)
            return 0;

        dspx::Track *currentTrack = nullptr;
        dspx::SingingClip *associatedSingingClip = nullptr;
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Track:
                currentTrack = selectionModel->trackSelectionModel()->currentItem();
                break;
            case dspx::SelectionModel::ST_Clip: {
                auto *clip = selectionModel->clipSelectionModel()->currentItem();
                auto *clipSequence = clip ? clip->clipSequence() : nullptr;
                currentTrack = clipSequence ? clipSequence->track() : nullptr;
                break;
            }
            case dspx::SelectionModel::ST_Note: {
                auto *noteSelectionModel = selectionModel->noteSelectionModel();
                auto *note = noteSelectionModel->currentItem();
                auto *noteSequence = note ? note->noteSequence()
                                          : noteSelectionModel->noteSequenceWithSelectedItems();
                associatedSingingClip = noteSequence ? noteSequence->singingClip() : nullptr;
                break;
            }
            case dspx::SelectionModel::ST_AnchorNode: {
                auto *anchorNodeSelectionModel = selectionModel->anchorNodeSelectionModel();
                auto *anchorNode = anchorNodeSelectionModel->currentItem();
                auto *anchorNodeSequence = anchorNode ? anchorNode->anchorNodeSequence()
                                                      : anchorNodeSelectionModel->anchorNodeSequenceWithSelectedItems();
                auto *parameter = anchorNodeSequence ? anchorNodeSequence->parameter() : nullptr;
                auto *parameterMap = parameter ? parameter->parameterMap() : nullptr;
                associatedSingingClip = parameterMap ? parameterMap->singingClip() : nullptr;
                break;
            }
            case dspx::SelectionModel::ST_DynamicMixingAnchor: {
                auto *anchorSelectionModel = selectionModel->dynamicMixingAnchorSelectionModel();
                auto *anchor = anchorSelectionModel->currentItem();
                auto *anchorSequence = anchor ? anchor->dynamicMixingAnchorSequence()
                                              : anchorSelectionModel->dynamicMixingAnchorSequenceWithSelectedItems();
                auto *sources = anchorSequence ? anchorSequence->sources() : nullptr;
                associatedSingingClip = sources ? sources->singingClip() : nullptr;
                break;
            }
            case dspx::SelectionModel::ST_None:
            case dspx::SelectionModel::ST_Label:
            case dspx::SelectionModel::ST_Tempo:
            case dspx::SelectionModel::ST_KeySignature:
                break;
        }

        if (!currentTrack && associatedSingingClip) {
            auto *clipSequence = associatedSingingClip->clipSequence();
            currentTrack = clipSequence ? clipSequence->track() : nullptr;
        }

        const auto tracks = model->tracks()->items();
        const int index = tracks.indexOf(currentTrack);
        return index >= 0 ? index : 0;
    }

    bool DspxDocumentPrivate::updatePasteAvailable() {
        const auto type = currentClipboardType();
        bool value = false;
        if (type.has_value()) {
            if (auto *clipboard = DspxClipboard::instance()) {
                value = clipboard->availablePasteTypes().contains(type.value());
            }
        }
        return emitOnChange(value, pasteAvailable, &DspxDocument::pasteAvailableChanged);
    }

    std::optional<DspxClipboardData::Type> DspxDocumentPrivate::currentClipboardType() const {
        if (freeParameterSelectionModel->isActive())
            return DspxClipboardData::FreeParameter;
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Tempo:
                return DspxClipboardData::Tempo;
            case dspx::SelectionModel::ST_Label:
                return DspxClipboardData::Label;
            case dspx::SelectionModel::ST_KeySignature:
                return DspxClipboardData::KeySignature;
            case dspx::SelectionModel::ST_Track:
                return DspxClipboardData::Track;
            case dspx::SelectionModel::ST_Clip:
                return DspxClipboardData::Clip;
            case dspx::SelectionModel::ST_Note:
                return clipboardSingingClip() ? std::optional(DspxClipboardData::Note) : std::nullopt;
            case dspx::SelectionModel::ST_AnchorNode:
                return clipboardSingingClip() ? std::optional(DspxClipboardData::AnchorNode) : std::nullopt;
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                return clipboardSingingClip() ? std::optional(DspxClipboardData::DynamicMixingAnchor) : std::nullopt;
            case dspx::SelectionModel::ST_None:
                return std::nullopt;
        }
        return std::nullopt;
    }

    dspx::SingingClip *DspxDocumentPrivate::clipboardSingingClip() const {
        if (freeParameterSelectionModel->isActive())
            return freeParameterSelectionModel->singingClip();
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Note: {
                auto *sequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
                return sequence ? sequence->singingClip() : nullptr;
            }
            case dspx::SelectionModel::ST_AnchorNode: {
                auto *sequence = selectionModel->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems();
                auto *parameter = sequence ? sequence->parameter() : nullptr;
                auto *map = parameter ? parameter->parameterMap() : nullptr;
                return map ? map->singingClip() : nullptr;
            }
            case dspx::SelectionModel::ST_DynamicMixingAnchor: {
                auto *sequence = selectionModel->dynamicMixingAnchorSelectionModel()->dynamicMixingAnchorSequenceWithSelectedItems();
                auto *sources = sequence ? sequence->sources() : nullptr;
                return sources ? sources->singingClip() : nullptr;
            }
            default:
                return nullptr;
        }
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildClipboardData(int playheadPosition) const {
        const auto type = currentClipboardType();
        const auto *handler = type ? clipboardHandler(*type) : nullptr;
        return handler ? (this->*handler->copy)(playheadPosition) : std::nullopt;
    }

    bool DspxDocumentPrivate::copySelection(int playheadPosition) {
        auto *clipboard = DspxClipboard::instance();
        if (!clipboard)
            return false;
        const auto data = buildClipboardData(playheadPosition);
        if (!data)
            return false;
        clipboard->copy({*data}, nullptr);
        qCInfo(lcDspxDocument) << "The selection was copied. Type:" << data->type();
        updatePasteAvailable();
        return true;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildTempoClipboardData(int playheadPosition) const {
        return buildSequenceData(selectionModel->tempoSelectionModel()->selectedItems(), playheadPosition, &DspxClipboardData::setTempos);
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildLabelClipboardData(int playheadPosition) const {
        return buildSequenceData(selectionModel->labelSelectionModel()->selectedItems(), playheadPosition, &DspxClipboardData::setLabels);
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildKeySignatureClipboardData(int playheadPosition) const {
        return buildSequenceData(selectionModel->keySignatureSelectionModel()->selectedItems(), playheadPosition, &DspxClipboardData::setKeySignatures);
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildNoteClipboardData(int playheadPosition) const {
        auto *clip = clipboardSingingClip();
        if (!clip)
            return std::nullopt;
        return buildSequenceData(selectionModel->noteSelectionModel()->selectedItems(), playheadPosition - clip->start(), &DspxClipboardData::setNotes);
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildDynamicMixingAnchorClipboardData(int playheadPosition) const {
        auto *clip = clipboardSingingClip();
        if (!clip)
            return std::nullopt;
        return buildSequenceData(selectionModel->dynamicMixingAnchorSelectionModel()->selectedItems(), playheadPosition - clip->start(), &DspxClipboardData::setDynamicMixingAnchors);
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildAnchorNodeClipboardData(int playheadPosition) const {
        auto *clip = clipboardSingingClip();
        if (!clip)
            return std::nullopt;
        return buildSequenceData(selectionModel->anchorNodeSelectionModel()->selectedItems(), playheadPosition - clip->start(), &DspxClipboardData::setAnchorNodes);
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildFreeParameterClipboardData(int playheadPosition) const {
        if (!freeParameterSelectionModel->hasSelection())
            return std::nullopt;
        auto *clip = freeParameterSelectionModel->singingClip();
        auto *parameter = clip->parameters()->item(freeParameterSelectionModel->parameterId());
        const int start = freeParameterSelectionModel->start();
        const int end = freeParameterSelectionModel->end();
        FreeParameterClipboardData range {.length = end - start, .offset = -(start % dspx::FreeValueDataArray::step())};
        if (parameter) {
            auto *array = freeParameterSelectionModel->layer() == FreeParameterSelectionModel::TransformLayer
                ? parameter->freeTransform() : parameter->freeEdited();
            range = Internal::captureFreeParameter(array, start, end);
        }
        DspxClipboardData data;
        data.setFreeParameter(range);
        data.setAbsolute(start);
        data.setPlayhead(playheadPosition - clip->start() - start);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildTrackClipboardData(int) const {
        auto selectedItems = selectionModel->trackSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;
        const auto orderedTracks = model->tracks()->items();
        std::sort(selectedItems.begin(), selectedItems.end(), [&orderedTracks](auto *left, auto *right) {
            return orderedTracks.indexOf(left) < orderedTracks.indexOf(right);
        });
        QList<opendspx::Track> tracks;
        tracks.reserve(selectedItems.size());
        for (const auto *track : std::as_const(selectedItems))
            tracks.append(track->toOpenDSPX());
        DspxClipboardData data;
        data.setTracks(tracks);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildClipClipboardData(int playheadPosition) const {
        const auto selectedItems = selectionModel->clipSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;
        const auto tracks = model->tracks()->items();
        int firstTrack = static_cast<int>(tracks.size());
        int lastTrack = -1;
        int absolute = std::numeric_limits<int>::max();
        for (auto *clip : selectedItems) {
            const int track = tracks.indexOf(clip->clipSequence()->track());
            firstTrack = std::min(firstTrack, track);
            lastTrack = std::max(lastTrack, track);
            absolute = std::min(absolute, clip->position());
        }
        if (firstTrack < 0 || lastTrack < firstTrack)
            return std::nullopt;
        QList<std::vector<opendspx::ClipRef>> clips(lastTrack - firstTrack + 1);
        for (auto *clip : selectedItems) {
            auto value = clip->toOpenDSPX();
            value->time.pos -= absolute;
            clips[tracks.indexOf(clip->clipSequence()->track()) - firstTrack].push_back(std::move(value));
        }
        for (auto &trackClips : clips) {
            std::stable_sort(trackClips.begin(), trackClips.end(), [](const auto &left, const auto &right) {
                if (left->time.pos != right->time.pos)
                    return left->time.pos < right->time.pos;
                return left->time.clipLen < right->time.clipLen;
            });
        }
        DspxClipboardData data;
        data.setClips(clips);
        data.setAbsolute(absolute);
        data.setPlayhead(playheadPosition - absolute);
        data.setTrack(firstTrack);
        return data;
    }

    int DspxDocumentPrivate::pastePosition(const DspxClipboardData &data, int playheadPosition, DspxDocument::PasteMode mode) const {
        const auto *clip = clipboardSingingClip();
        const qint64 playhead = static_cast<qint64>(playheadPosition) - (clip ? clip->start() : 0);
        qint64 position = playhead;
        switch (mode) {
            case DspxDocument::CurrentPosition:
                break;
            case DspxDocument::RelativeToCopiedPlayhead:
                position -= data.playhead();
                break;
            case DspxDocument::OriginalPosition:
                position = data.absolute();
                break;
        }
        return static_cast<int>(std::clamp<qint64>(position, 0, std::numeric_limits<int>::max()));
    }

    bool DspxDocumentPrivate::pasteTempos(const DspxClipboardData &data, PasteContext &context) {
        return pasteSequenceData(data.tempos(), model->tempos(), model, context, [this] { return model->createTempo(); }, true);
    }

    bool DspxDocumentPrivate::pasteLabels(const DspxClipboardData &data, PasteContext &context) {
        return pasteSequenceData(data.labels(), model->labels(), model, context, [this] { return model->createLabel(); }, false);
    }

    bool DspxDocumentPrivate::pasteKeySignatures(const DspxClipboardData &data, PasteContext &context) {
        return pasteSequenceData(data.keySignatures(), model->keySignatures(), model, context, [this] { return model->createKeySignature(); }, true);
    }

    bool DspxDocumentPrivate::pasteNotes(const DspxClipboardData &data, PasteContext &context) {
        auto *sequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
        return pasteSequenceData(data.notes(), sequence, model, context, [this] { return model->createNote(); }, false);
    }

    bool DspxDocumentPrivate::pasteDynamicMixingAnchors(const DspxClipboardData &data, PasteContext &context) {
        auto *sequence = selectionModel->dynamicMixingAnchorSelectionModel()->dynamicMixingAnchorSequenceWithSelectedItems();
        if (!sequence || !sequence->sources())
            return false;
        const int singerCount = sequence->sources()->singers()->size();
        if (singerCount <= 0)
            return false;
        auto anchors = data.dynamicMixingAnchors();
        for (auto &anchor : anchors)
            anchor.ratio.resize(static_cast<std::size_t>(singerCount - 1), 0.0);
        return pasteSequenceData(std::move(anchors), sequence, model, context, [this] { return model->createDynamicMixingAnchor(); }, true);
    }

    bool DspxDocumentPrivate::pasteAnchorNodes(const DspxClipboardData &data, PasteContext &context) {
        auto *sequence = selectionModel->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems();
        return pasteSequenceData(data.anchorNodes(), sequence, model, context, [this] { return model->createAnchorNode(); }, true);
    }

    bool DspxDocumentPrivate::pasteTracks(const DspxClipboardData &data, PasteContext &context) {
        auto *list = model->tracks();
        int insertionIndex = list->size();
        if (auto *current = selectionModel->trackSelectionModel()->currentItem()) {
            const int index = list->items().indexOf(current);
            if (index >= 0)
                insertionIndex = index + 1;
        }
        for (const auto &value : data.tracks()) {
            auto *track = model->createTrack();
            track->fromOpenDSPX(value);
            if (!list->insertItem(insertionIndex++, track)) {
                model->destroyItem(track);
                return false;
            }
            context.items.append(track);
        }
        return !context.items.isEmpty();
    }

    bool DspxDocumentPrivate::pasteClips(const DspxClipboardData &data, PasteContext &context) {
        const auto clips = data.clips();
        const bool hasClip = std::ranges::any_of(clips, [](const auto &trackClips) {
            return std::ranges::any_of(trackClips, [](const auto &clip) { return bool(clip); });
        });
        if (!hasClip)
            return false;
        const int firstTrack = std::max(0, context.mode == DspxDocument::OriginalPosition ? data.track() : currentTrackIndex());
        auto *tracks = model->tracks();
        if (clips.size() > std::numeric_limits<int>::max() - firstTrack)
            return false;
        const int requiredTrackCount = firstTrack + static_cast<int>(clips.size());
        while (tracks->size() < requiredTrackCount) {
            auto *track = model->createTrack();
            track->fromOpenDSPX(opendspx::Track {});
            if (!tracks->insertItem(tracks->size(), track)) {
                model->destroyItem(track);
                return false;
            }
        }
        for (int index = 0; index < clips.size(); ++index) {
            auto *sequence = tracks->item(firstTrack + index)->clips();
            for (const auto &value : clips.at(index)) {
                if (!value)
                    continue;
                const qint64 position = static_cast<qint64>(value->time.pos) + context.position;
                if (position > std::numeric_limits<int>::max())
                    return false;
                dspx::Clip *clip = nullptr;
                switch (value->type) {
                    case opendspx::Clip::Type::Audio:
                        clip = model->createAudioClip();
                        break;
                    case opendspx::Clip::Type::Singing:
                        clip = model->createSingingClip();
                        break;
                }
                if (!clip)
                    return false;
                clip->fromOpenDSPX(value);
                clip->setPosition(static_cast<int>(position));
                if (!sequence->insertItem(clip)) {
                    model->destroyItem(clip);
                    return false;
                }
                context.items.append(clip);
            }
        }
        return !context.items.isEmpty();
    }

    bool DspxDocumentPrivate::pasteFreeParameter(const DspxClipboardData &data, PasteContext &context) {
        const auto range = data.freeParameter();
        if (range.length > std::numeric_limits<int>::max() - context.position)
            return false;
        const auto sampled = Internal::resampleFreeParameter(range, context.position);
        if (sampled.values.isEmpty())
            return false;
        auto *clip = freeParameterSelectionModel->singingClip();
        auto *parameters = clip->parameters();
        const auto id = freeParameterSelectionModel->parameterId();
        auto *parameter = parameters->item(id);
        const bool hasValues = std::ranges::any_of(sampled.values, [](const auto &value) { return value.isValid(); });
        if (!parameter && !hasValues)
            return false;
        if (!parameter) {
            parameter = model->createParameter();
            if (!parameters->insertItem(id, parameter)) {
                model->destroyItem(parameter);
                return false;
            }
        }
        auto *array = freeParameterSelectionModel->layer() == FreeParameterSelectionModel::TransformLayer
            ? parameter->freeTransform() : parameter->freeEdited();
        auto current = array->slice(sampled.firstIndex, sampled.values.size());
        current.resize(sampled.values.size());
        if (current == sampled.values)
            return false;
        const int endIndex = sampled.firstIndex + static_cast<int>(sampled.values.size());
        if (array->size() < endIndex && !array->splice(array->size(), 0, QList<QVariant>(endIndex - array->size())))
            return false;
        if (!array->splice(sampled.firstIndex, sampled.values.size(), sampled.values))
            return false;
        context.rangeStart = context.position;
        context.rangeEnd = context.position + range.length;
        return true;
    }

    bool DspxDocumentPrivate::canSafelyBounceIgnoringResampling() const {
        const auto trackBounceInfoMap = buildTrackBounceInfoMap(selectionModel);
        if (trackBounceInfoMap.isEmpty())
            return false;

        for (const auto &info : trackBounceInfoMap) {
            ParameterPointOccupancy occupiedParameters;
            QSet<int> occupiedDynamicMixingAnchors;
            const bool singerLayoutIdentical = bounceInfoSingerLayoutIdentical(info);
            for (auto *clip : info.clips) {
                const int clipStart = clip->clipStart();
                const int clipEnd = clipStart + clip->clipLength();
                const int deltaPosition = clip->start() - info.bouncedClipTime.pos;

                const int noteSliceLength = std::max(1, clip->clipLength());
                for (auto *note : clip->notes()->slice(clipStart, noteSliceLength)) {
                    const int noteStart = note->position();
                    const int noteEnd = noteStart + note->length();
                    if (noteEnd <= clipStart || noteStart >= clipEnd)
                        continue;
                    if (noteStart < clipStart || noteEnd > clipEnd)
                        return false;
                }

                for (const auto &parameterId : clip->parameters()->keys()) {
                    auto *parameter = clip->parameters()->item(parameterId);
                    if (!parameter)
                        continue;
                    if (!freeValuesCanSafelyBounce(parameter->freeTransform(), parameterId,
                                                   clipStart, clipEnd, deltaPosition,
                                                   occupiedParameters.freeTransform) ||
                        !freeValuesCanSafelyBounce(parameter->freeEdited(), parameterId,
                                                   clipStart, clipEnd, deltaPosition,
                                                   occupiedParameters.freeEdited) ||
                        !anchorSequenceCanSafelyBounce(parameter->anchorTransform(), parameterId,
                                                       clipStart, clipEnd, deltaPosition,
                                                       occupiedParameters.anchorTransform) ||
                        !anchorSequenceCanSafelyBounce(parameter->anchorEdited(), parameterId,
                                                       clipStart, clipEnd, deltaPosition,
                                                       occupiedParameters.anchorEdited)) {
                        return false;
                    }
                }

                if (singerLayoutIdentical && clip->sources() &&
                    !dynamicMixingCanSafelyBounce(clip->sources()->dynamicMixingAnchors(),
                                                  clipStart, clipEnd, deltaPosition,
                                                  occupiedDynamicMixingAnchors)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool DspxDocumentPrivate::bounceRequiresResampling() const {
        const auto trackBounceInfoMap = buildTrackBounceInfoMap(selectionModel);
        for (const auto &info : trackBounceInfoMap) {
            for (auto *clip : info.clips) {
                const int start = clip->clipStart();
                const int end = start + clip->clipLength();
                const int targetStart = clip->position() - info.bouncedClipTime.pos;
                for (const auto &id : clip->parameters()->keys()) {
                    auto *parameter = clip->parameters()->item(id);
                    if (!parameter)
                        continue;
                    if (Internal::freeParameterRequiresResampling(Internal::captureFreeParameter(parameter->freeTransform(), start, end), targetStart) ||
                        Internal::freeParameterRequiresResampling(Internal::captureFreeParameter(parameter->freeEdited(), start, end), targetStart)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool DspxDocumentPrivate::deleteSelection() {
        if (freeParameterSelectionModel && freeParameterSelectionModel->hasSelection())
            return deleteFreeParameterSelection();
        if (!selectionModel || selectionModel->selectedCount() <= 0)
            return false;

        int removedCount = 0;
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Tempo:
                removedCount = deleteTempos();
                break;
            case dspx::SelectionModel::ST_Label:
                removedCount = deleteLabels();
                break;
            case dspx::SelectionModel::ST_KeySignature:
                removedCount = deleteKeySignatures();
                break;
            case dspx::SelectionModel::ST_Track:
                removedCount = deleteTracks();
                break;
            case dspx::SelectionModel::ST_Clip:
                removedCount = deleteClips();
                break;
            case dspx::SelectionModel::ST_Note:
                removedCount = deleteNotes();
                break;
            case dspx::SelectionModel::ST_AnchorNode:
                removedCount = deleteAnchorNodes();
                break;
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                removedCount = deleteDynamicMixingAnchors();
                break;
            case dspx::SelectionModel::ST_None:
            default:
                break;
        }

        if (removedCount > 0)
            qCInfo(lcDspxDocument) << "Deleted selection" << selectionTypeName(selectionModel->selectionType()) << "count" << removedCount;
        else
            qCDebug(lcDspxDocument) << "Delete selection produced no changes" << selectionTypeName(selectionModel->selectionType());

        return removedCount > 0;
    }

    int DspxDocumentPrivate::deleteTempos() {
        auto *tempoSequence = model->tempos();
        int removedCount = 0;
        for (auto *item : selectionModel->tempoSelectionModel()->selectedItems()) {
            // Protect the initial tempo at position 0 when it is the only one.
            if (item->position() == 0) {
                const auto overlappingItems = tempoSequence->slice(0, 1);
                if (overlappingItems.size() == 1)
                    continue;
            }

            if (tempoSequence->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DspxDocumentPrivate::deleteLabels() {
        auto *labelSequence = model->labels();
        int removedCount = 0;
        for (auto *item : selectionModel->labelSelectionModel()->selectedItems()) {
            if (labelSequence->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DspxDocumentPrivate::deleteKeySignatures() {
        auto *keySignatureSequence = model->keySignatures();
        int removedCount = 0;
        for (auto *item : selectionModel->keySignatureSelectionModel()->selectedItems()) {
            // Note: KeySignature allows deletion at position 0, unlike Tempo
            if (keySignatureSequence->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DspxDocumentPrivate::deleteTracks() {
        auto *trackList = model->tracks();
        const auto allTracks = trackList->items();
        QList<int> indexes;
        for (auto *item : selectionModel->trackSelectionModel()->selectedItems()) {
            const int index = allTracks.indexOf(item);
            if (index >= 0)
                indexes.append(index);
        }

        std::sort(indexes.begin(), indexes.end(), [](int lhs, int rhs) {
            return lhs > rhs;
        });

        int removedCount = 0;
        for (const int index : std::as_const(indexes)) {
            auto *removedTrack = trackList->removeItem(index);
            if (removedTrack) {
                model->destroyItem(removedTrack);
                ++removedCount;
            }
        }

        return removedCount;
    }

    int DspxDocumentPrivate::deleteClips() {
        int removedCount = 0;
        for (auto *item : selectionModel->clipSelectionModel()->selectedItems()) {
            if (item->clipSequence()->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DspxDocumentPrivate::deleteNotes() {
        int removedCount = 0;
        for (auto *item : selectionModel->noteSelectionModel()->selectedItems()) {
            if (item->noteSequence()->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DspxDocumentPrivate::deleteAnchorNodes() {
        auto *anchorSelectionModel = selectionModel->anchorNodeSelectionModel();
        auto *anchorSequence = anchorSelectionModel->anchorNodeSequenceWithSelectedItems();
        if (!anchorSequence)
            return 0;

        int removedCount = 0;
        for (auto *item : anchorSelectionModel->selectedItems()) {
            if (anchorSequence->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DspxDocumentPrivate::deleteDynamicMixingAnchors() {
        auto *dynamicSelection = selectionModel->dynamicMixingAnchorSelectionModel();
        auto *sequence = dynamicSelection->dynamicMixingAnchorSequenceWithSelectedItems();
        if (!sequence)
            return 0;

        int removedCount = 0;
        for (auto *item : dynamicSelection->selectedItems()) {
            if (sequence->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    bool DspxDocumentPrivate::deleteFreeParameterSelection() {
        if (!freeParameterSelectionModel || !freeParameterSelectionModel->hasSelection())
            return false;
        auto *clip = freeParameterSelectionModel->singingClip();
        if (!clip)
            return false;
        auto *parameter = clip->parameters()->item(freeParameterSelectionModel->parameterId());
        if (!parameter)
            return false;

        auto *values = freeParameterSelectionModel->layer() == FreeParameterSelectionModel::TransformLayer
            ? parameter->freeTransform() : parameter->freeEdited();
        const int step = dspx::FreeValueDataArray::step();
        const int firstIndex = (freeParameterSelectionModel->start() + step - 1) / step;
        const int endIndex = (freeParameterSelectionModel->end() + step - 1) / step;
        const int boundedFirstIndex = std::clamp(firstIndex, 0, values->size());
        const int boundedEndIndex = std::clamp(endIndex, boundedFirstIndex, values->size());
        const int count = boundedEndIndex - boundedFirstIndex;
        if (count <= 0)
            return false;

        const auto currentValues = values->slice(boundedFirstIndex, count);
        if (std::none_of(currentValues.cbegin(), currentValues.cend(), [](const QVariant &value) {
                return value.isValid();
            })) {
            return false;
        }

        return values->splice(boundedFirstIndex, count, QList<QVariant>(count));
    }

    void DspxDocumentPrivate::selectAllTempos() {
        for (auto item : model->tempos()->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DspxDocumentPrivate::selectAllLabels() {
        for (auto item : model->labels()->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DspxDocumentPrivate::selectAllKeySignatures() {
        for (auto item : model->keySignatures()->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DspxDocumentPrivate::selectAllTracks() {
        for (auto item : model->tracks()->items()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DspxDocumentPrivate::selectAllClips() {
        for (auto track : model->tracks()->items()) {
            for (auto item : track->clips()->asRange()) {
                selectionModel->select(item, dspx::SelectionModel::Select);
            }
        }
    }

    void DspxDocumentPrivate::selectAllNotes() {
        auto noteSequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
        if (!noteSequence)
            return;
        for (auto item : noteSequence->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DspxDocumentPrivate::selectAllAnchorNodes() {
        auto *anchorSelectionModel = selectionModel->anchorNodeSelectionModel();
        auto *anchorSequence = anchorSelectionModel->anchorNodeSequenceWithSelectedItems();
        if (!anchorSequence)
            return;
        for (auto *item : anchorSequence->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DspxDocumentPrivate::selectAllDynamicMixingAnchors() {
        auto *dynamicSelection = selectionModel->dynamicMixingAnchorSelectionModel();
        auto *sequence = dynamicSelection->dynamicMixingAnchorSequenceWithSelectedItems();
        if (!sequence)
            return;
        for (auto *item : sequence->asRange())
            selectionModel->select(item, dspx::SelectionModel::Select);
    }

    void DspxDocumentPrivate::selectAllFreeParameter() {
        if (!freeParameterSelectionModel || !freeParameterSelectionModel->isActive())
            return;
        auto *clip = freeParameterSelectionModel->singingClip();
        if (!clip)
            return;
        freeParameterSelectionModel->setRange(clip->clipStart(), clip->clipStart() + clip->clipLength());
    }

    DspxDocument::DspxDocument(QObject *parent) : QObject(parent), d_ptr(new DspxDocumentPrivate) {
        Q_D(DspxDocument);
        d->q_ptr = this;
        d->dspxDocument = new dspx::Document(this);
        d->model = new dspx::Model(d->dspxDocument, this);
        d->selectionModel = new dspx::SelectionModel(d->model, this);
        d->freeParameterSelectionModel = new FreeParameterSelectionModel(d->selectionModel, this);
        auto transactionalStrategy = new TransactionalModelStrategy(d->dspxDocument);
        d->transactionController = new TransactionController(transactionalStrategy, this);
        transactionalStrategy->setParent(d->transactionController);

        d->initConnections();
    }

    DspxDocument::~DspxDocument() = default;

    dspx::Model *DspxDocument::model() const {
        Q_D(const DspxDocument);
        return d->model;
    }

    dspx::SelectionModel *DspxDocument::selectionModel() const {
        Q_D(const DspxDocument);
        return d->selectionModel;
    }

    FreeParameterSelectionModel *DspxDocument::freeParameterSelectionModel() const {
        Q_D(const DspxDocument);
        return d->freeParameterSelectionModel;
    }

    TransactionController *DspxDocument::transactionController() const {
        Q_D(const DspxDocument);
        return d->transactionController;
    }

    bool DspxDocument::anyItemsSelected() {
        Q_D(DspxDocument);
        return d->anyItemsSelected;
    }

    bool DspxDocument::isEditScopeFocused() {
        Q_D(DspxDocument);
        return d->editScopeFocused;
    }

    bool DspxDocument::pasteAvailable() {
        Q_D(DspxDocument);
        return d->pasteAvailable;
    }

    void DspxDocument::cutSelection(int playheadPosition) {
        Q_D(DspxDocument);
        if (anyItemsSelected() && d->copySelection(playheadPosition))
            deleteSelection();
    }

    void DspxDocument::copySelection(int playheadPosition) {
        Q_D(DspxDocument);
        if (anyItemsSelected())
            d->copySelection(playheadPosition);
    }

    void DspxDocument::paste(int playheadPosition, PasteMode pasteMode) {
        Q_D(DspxDocument);
        const auto type = d->currentClipboardType();
        auto *clipboard = DspxClipboard::instance();
        const auto *handler = type ? clipboardHandler(*type) : nullptr;
        if (!clipboard || !handler || !pasteAvailable())
            return;
        bool ok = false;
        const auto data = clipboard->paste(*type, &ok);
        if (!ok) {
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), nullptr, tr("Paste Failed"), tr("Cannot paste data from the clipboard."));
            return;
        }
        DspxDocumentPrivate::PasteContext context {
            .position = d->pastePosition(data, playheadPosition, pasteMode),
            .mode = pasteMode,
        };
        if (data.type() == DspxClipboardData::FreeParameter) {
            const QPointer<DspxDocument> documentGuard(this);
            const QPointer<dspx::SingingClip> targetClip(d->freeParameterSelectionModel->singingClip());
            const auto parameterId = d->freeParameterSelectionModel->parameterId();
            const auto layer = d->freeParameterSelectionModel->layer();
            ParameterResamplingScenario scenario;
            scenario.setWindow(qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow()));
            if (!scenario.confirmPaste(data.freeParameter(), context.position))
                return;
            if (!documentGuard || !targetClip || d->freeParameterSelectionModel->singingClip() != targetClip ||
                d->freeParameterSelectionModel->parameterId() != parameterId ||
                d->freeParameterSelectionModel->layer() != layer) {
                return;
            }
        }
        bool pasted = false;
        const bool started = transactionController()->beginScopedTransaction(tr(handler->transactionName), [&] {
            pasted = (d->*handler->paste)(data, context);
            return pasted;
        }, [] {
            qCCritical(lcDspxDocument) << "The paste transaction could not be started.";
        });
        if (!started || !pasted)
            return;
        if (data.type() == DspxClipboardData::FreeParameter) {
            d->freeParameterSelectionModel->setRange(context.rangeStart, context.rangeEnd);
        } else {
            bool first = true;
            for (auto *item : std::as_const(context.items)) {
                auto command = dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem;
                if (first)
                    command |= dspx::SelectionModel::ClearPreviousSelection;
                d->selectionModel->select(item, command, dspx::SelectionModel::selectionTypeFromItem(item));
                first = false;
            }
        }
        qCInfo(lcDspxDocument) << "The selection was pasted. Type:" << data.type() << "Mode:" << pasteMode;
    }

    void DspxDocument::deleteSelection() {
        Q_D(DspxDocument);
        if (!anyItemsSelected())
            return;
        const bool deletingFreeParameter = d->freeParameterSelectionModel && d->freeParameterSelectionModel->hasSelection();
        const auto selectionType = d->selectionModel ? d->selectionModel->selectionType() : dspx::SelectionModel::ST_None;
        const QString transactionName = [selectionType, deletingFreeParameter, this] {
            if (deletingFreeParameter)
                return tr("Deleting parameter data");
            switch (selectionType) {
                case dspx::SelectionModel::ST_Tempo:
                    return tr("Deleting tempo");
                case dspx::SelectionModel::ST_Label:
                    return tr("Deleting label");
                case dspx::SelectionModel::ST_KeySignature:
                    return tr("Deleting key signature");
                case dspx::SelectionModel::ST_Track:
                    return tr("Deleting track");
                case dspx::SelectionModel::ST_Clip:
                    return tr("Deleting clip");
                case dspx::SelectionModel::ST_Note:
                    return tr("Deleting note");
                case dspx::SelectionModel::ST_AnchorNode:
                    return tr("Deleting anchor node");
                case dspx::SelectionModel::ST_DynamicMixingAnchor:
                    return tr("Deleting voice blending anchor");
                case dspx::SelectionModel::ST_None:
                default:
                    return tr("Deleting selection");
            }
        }();

        qCInfo(lcDspxDocument) << "Delete selection" << selectionTypeName(selectionType);
        bool deleted = false;
        transactionController()->beginScopedTransaction(transactionName, [=, &deleted] {
            deleted = d->deleteSelection();
            return deleted; }, [] { qCCritical(lcDspxDocument) << "Failed to delete selection in scoped transaction"; });
        if (deletingFreeParameter)
            d->freeParameterSelectionModel->clear();
    }

    void DspxDocument::selectAll() {
        Q_D(DspxDocument);
        if (!isEditScopeFocused()) {
            return;
        }
        if (d->freeParameterSelectionModel && d->freeParameterSelectionModel->isActive()) {
            qCInfo(lcDspxDocument) << "Select all free parameter data";
            d->selectAllFreeParameter();
            return;
        }
        qCInfo(lcDspxDocument) << "Select all" << selectionTypeName(d->selectionModel->selectionType());
        switch (d->selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Tempo:
                d->selectAllTempos();
                break;
            case dspx::SelectionModel::ST_Label:
                d->selectAllLabels();
                break;
            case dspx::SelectionModel::ST_KeySignature:
                d->selectAllKeySignatures();
                break;
            case dspx::SelectionModel::ST_Track:
                d->selectAllTracks();
                break;
            case dspx::SelectionModel::ST_Clip:
                d->selectAllClips();
                break;
            case dspx::SelectionModel::ST_Note:
                d->selectAllNotes();
                break;
            case dspx::SelectionModel::ST_AnchorNode:
                d->selectAllAnchorNodes();
                break;
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                d->selectAllDynamicMixingAnchors();
                break;
            case dspx::SelectionModel::ST_None:
            default:
                break;
        }
    }

    void DspxDocument::deselectAll() {
        Q_D(DspxDocument);
        if (!anyItemsSelected()) {
            return;
        }
        if (d->freeParameterSelectionModel && d->freeParameterSelectionModel->hasSelection()) {
            qCInfo(lcDspxDocument) << "Deselect free parameter range";
            d->freeParameterSelectionModel->clear();
            return;
        }
        qCInfo(lcDspxDocument) << "Deselect all" << selectionTypeName(d->selectionModel->selectionType());
        d->selectionModel->select(nullptr, dspx::SelectionModel::ClearPreviousSelection, d->selectionModel->selectionType());
    }

    void DspxDocument::splitItems(int position) {
        Q_D(DspxDocument);
        const auto selectionType = d->selectionModel->selectionType();
        if (selectionType != dspx::SelectionModel::ST_Clip && selectionType != dspx::SelectionModel::ST_Note)
            return;

        const QString transactionName = selectionType == dspx::SelectionModel::ST_Clip ? tr("Splitting clip") : tr("Splitting note");

        bool changed = false;
        transactionController()->beginScopedTransaction(transactionName, [=, &changed] {
            switch (selectionType) {
                case dspx::SelectionModel::ST_Clip: {
                    const auto selectedClips = d->selectionModel->clipSelectionModel()->selectedItems();
                    for (auto *clip : selectedClips) {
                        const int clipPos = clip->position();
                        const int clipEnd = clipPos + clip->clipLength();
                        if (position <= clipPos || position >= clipEnd)
                            continue;

                        const auto data = clip->toOpenDSPX();
                        const int newClipLength = position - clipPos;
                        clip->setClipLength(newClipLength);
                        data->time.clipLen -= newClipLength;
                        data->time.clipStart = position - (data->time.pos - data->time.clipStart);
                        data->time.pos = position;

                        dspx::Clip *newClip = nullptr;
                        switch (clip->kind()) {
                            case dspx::Clip::Audio:
                                newClip = d->model->createAudioClip();
                                break;
                            case dspx::Clip::Singing:
                                newClip = d->model->createSingingClip();
                                break;
                        }

                        newClip->fromOpenDSPX(data);
                        auto *clipSequence = clip->clipSequence();
                        if (!clipSequence || !clipSequence->insertItem(newClip)) {
                            d->model->destroyItem(newClip);
                            continue;
                        }

                        changed = true;
                        d->selectionModel->select(newClip, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem);
                    }
                    break;
                }
                case dspx::SelectionModel::ST_Note: {
                    const auto selectedNotes = d->selectionModel->noteSelectionModel()->selectedItems();
                    for (auto *note : selectedNotes) {
                        const int notePos = note->position();
                        const int noteEnd = notePos + note->length();
                        if (position <= notePos || position >= noteEnd)
                            continue;

                        auto data = note->toOpenDSPX();
                        const int newNoteLength = position - notePos;
                        note->setLength(newNoteLength);
                        data.length -= newNoteLength;
                        data.pos = position;

                        auto *newNote = d->model->createNote();
                        newNote->fromOpenDSPX(data);
                        newNote->setLyric("-");
                        auto *noteSequence = note->noteSequence();
                        if (!noteSequence || !noteSequence->insertItem(newNote)) {
                            d->model->destroyItem(newNote);
                            continue;
                        }

                        changed = true;
                        d->selectionModel->select(newNote,
                            dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem,
                            dspx::SelectionModel::selectionTypeFromItem(newNote));
                    }
                    break;
                }
                default:
                    break;
            }
            return changed;
        }, [] {
            qCCritical(lcDspxDocument) << "Failed to split items in scoped transaction";
        });
    }

    bool DspxDocument::isClipSingerLayoutIdentical() const {
        Q_D(const DspxDocument);
        const auto trackBounceInfoMap = buildTrackBounceInfoMap(d->selectionModel);
        if (trackBounceInfoMap.isEmpty())
            return false;
        return std::all_of(trackBounceInfoMap.cbegin(), trackBounceInfoMap.cend(), [](const BounceInfo &info) {
            return bounceInfoSingerLayoutIdentical(info);
        });
    }

    bool DspxDocument::canSafelyBounce() const {
        Q_D(const DspxDocument);
        return d->canSafelyBounceIgnoringResampling() && !d->bounceRequiresResampling();
    }

    void DspxDocument::bounceToClip() {
        Q_D(DspxDocument);
        qCInfo(lcDspxDocument) << "Bouncing to clip";
        const auto trackBounceInfoMap = buildTrackBounceInfoMap(d->selectionModel);
        if (trackBounceInfoMap.isEmpty()) {
            qCDebug(lcDspxDocument) << "No singing-clips selected, ignoring this operation";
            return;
        }
        d->transactionController->beginScopedTransaction(tr("Bouncing to clip"), [&] {
            qCDebug(lcDspxDocument) << "Merging and rebounding clips";
            for (const auto &info : trackBounceInfoMap) {
                const auto mergedParameters = mergeParameterData(info);
                const bool singerLayoutIdentical = bounceInfoSingerLayoutIdentical(info);
                const auto mergedDynamicMixing = singerLayoutIdentical
                                                     ? mergeDynamicMixingData(info)
                                                     : QMap<int, QList<double>> {};
                QList<dspx::Note *> notes;
                // Take all notes and reposition notes
                for (auto *clip : info.clips) {
                    const auto deltaPosition = clip->start() - info.bouncedClipTime.pos;
                    // It is needed to copy all notes to a new list, because notes will be removed while iterating
                    // asRange() returns an enable borrowed range so it's safe to get iterator from prvalue
                    for (auto *note : QList(clip->notes()->asRange().cbegin(), clip->notes()->asRange().cend())) {
                        clip->notes()->removeItem(note);
                        if (note->position() + note->length() <= clip->clipStart() || note->position() >= clip->clipStart() + clip->clipLength()) {
                            d->model->destroyItem(note);
                            continue;
                        } else if (note->position() < clip->clipStart() || note->position() + note->length() > clip->clipStart() + clip->clipLength()) {
                            auto left = std::max(note->position(), clip->clipStart());
                            auto right = std::min(note->position() + note->length(), clip->clipStart() + clip->clipLength());
                            note->setPosition(left);
                            note->setLength(right - left);
                        }
                        note->setPosition(note->position() + deltaPosition);
                        notes.append(note);
                    }
                }
                // Insert notes into pivot clip
                for (auto note : notes) {
                    info.pivotClip->notes()->insertItem(note);
                }
                applyMergedParameters(info.pivotClip, mergedParameters, d->model);
                if (singerLayoutIdentical) {
                    applyMergedDynamicMixing(info.pivotClip, mergedDynamicMixing, d->model);
                } else if (auto *sources = info.pivotClip->sources()) {
                    clearDynamicMixingAnchors(sources->dynamicMixingAnchors(), d->model);
                }
                // Update time of pivot clip and delete non-pivot clips
                for (auto *clip : info.clips) {
                    if (clip == info.pivotClip) {
                        clip->setPosition(info.bouncedClipTime.pos);
                        clip->setLength(info.bouncedClipTime.length);
                        clip->setClipStart(info.bouncedClipTime.clipStart);
                        clip->setClipLength(info.bouncedClipTime.clipLen);
                    } else {
                        clip->clipSequence()->removeItem(clip);
                        d->model->destroyItem(clip);
                    }
                }
            }
            return true;
        }, [&] {
            qCCritical(lcDspxDocument) << "Failed to begin transaction";
        });
        qCInfo(lcDspxDocument) << "End bouncing to clip";
    }

    void DspxDocument::loadModel(const opendspx::Model &model) {
        Q_D(DspxDocument);
        auto transaction = d->dspxDocument->engine()->beginTransaction({.undoable = false});
        d->dspxDocument->setTransaction(&transaction);
        try {
            d->model->fromOpenDspx(model);
            transaction.commit();
            d->dspxDocument->setTransaction(nullptr);
        } catch (...) {
            d->dspxDocument->setTransaction(nullptr);
            throw;
        }
    }

}

#include "moc_DspxDocument.cpp"
