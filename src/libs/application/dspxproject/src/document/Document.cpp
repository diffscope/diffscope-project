// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Document.h"
#include "Document_p.h"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <QHash>
#include <QLoggingCategory>
#include <QMap>
#include <QSet>
#include <QVariant>

#include <opendspx/clip.h>
#include <opendspx/mixedsinger.h>
#include <opendspx/singlesinger.h>
#include <opendspx/sources.h>
#include <opendspx/track.h>

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
#include <dspxmodelSelectionModel/FreeParameterSelectionModel.h>
#include <dspxmodelSelectionModel/KeySignatureSelectionModel.h>
#include <dspxmodelSelectionModel/LabelSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelSelectionModel/TempoSelectionModel.h>
#include <dspxmodelSelectionModel/TrackSelectionModel.h>

#include <dspxproject/private/FreeParameterResampling_p.h>

#include <singercatalog/ArchitectureInfo.h>
#include <stdcorelib/support/json.h>
#include <transactional/TransactionController.h>
#include <transactional/TransactionalStrategy.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcDocument, "diffscope.dspxproject.dspxdocument")

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
            case dspx::SelectionModel::ST_FreeParameter:
                return "FreeParameter";
        }
        return "Unknown";
    }

    namespace {

        struct ClipboardHandler {
            ClipboardData::Type type;
            std::optional<ClipboardData> (DocumentPrivate::*copy)(int) const;
            bool (DocumentPrivate::*paste)(const ClipboardData &, DocumentPrivate::PasteContext &);
        };

        const ClipboardHandler &clipboardHandler(ClipboardData::Type type) {
            static const auto handlers = std::to_array<ClipboardHandler>({
                {ClipboardData::Tempo, &DocumentPrivate::buildTempoClipboardData, &DocumentPrivate::pasteTempos},
                {ClipboardData::Label, &DocumentPrivate::buildLabelClipboardData, &DocumentPrivate::pasteLabels},
                {ClipboardData::KeySignature, &DocumentPrivate::buildKeySignatureClipboardData, &DocumentPrivate::pasteKeySignatures},
                {ClipboardData::Track, &DocumentPrivate::buildTrackClipboardData, &DocumentPrivate::pasteTracks},
                {ClipboardData::Clip, &DocumentPrivate::buildClipClipboardData, &DocumentPrivate::pasteClips},
                {ClipboardData::Note, &DocumentPrivate::buildNoteClipboardData, &DocumentPrivate::pasteNotes},
                {ClipboardData::DynamicMixingAnchor, &DocumentPrivate::buildDynamicMixingAnchorClipboardData, &DocumentPrivate::pasteDynamicMixingAnchors},
                {ClipboardData::AnchorNode, &DocumentPrivate::buildAnchorNodeClipboardData, &DocumentPrivate::pasteAnchorNodes},
                {ClipboardData::FreeParameter, &DocumentPrivate::buildFreeParameterClipboardData, &DocumentPrivate::pasteFreeParameter},
            });
            const auto it = std::ranges::find(handlers, type, &ClipboardHandler::type);
            return *it;
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
        std::optional<ClipboardData> buildSequenceData(const QList<Item *> &items, int playhead, Setter setter) {
            using Data = decltype(items.first()->toOpenDSPX());
            QList<Data> values;
            values.reserve(items.size());
            for (const auto *item : items)
                values.append(item->toOpenDSPX());
            std::stable_sort(values.begin(), values.end(), itemPrecedes<Data>);
            const int absolute = itemPosition(values.first());
            for (auto &value : values)
                setItemPosition(value, itemPosition(value) - absolute);
            ClipboardData data;
            (data.*setter)(values);
            data.setAbsolute(absolute);
            data.setPlayhead(playhead - absolute);
            return data;
        }

        template<typename Data, typename Sequence, typename Factory>
        bool pasteSequenceData(QList<Data> values, Sequence *sequence, dspx::Model *model,
                               DocumentPrivate::PasteContext &context, Factory createItem, bool replaceOverlaps) {
            if (values.isEmpty())
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
            if (selectionModel->selectionType() != dspx::SelectionModel::ST_Clip)
                return result;

            const auto *preferredNoteSequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
            for (auto *clip : selectionModel->clipSelectionModel()->selectedItems()) {
                auto *singingClip = qobject_cast<dspx::SingingClip *>(clip);
                if (!singingClip)
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
            auto *sources = clip->sources();
            return sources ? sources->singers()->toOpenDSPX() : std::vector<opendspx::SingerRef> {};
        }

        bool bounceInfoSingerLayoutIdentical(const BounceInfo &info) {
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
            const auto data = FreeParameterResampling::capture(array, clipStart, clipEnd);
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
            const auto sampled = FreeParameterResampling::resample(data, clipStart + deltaPosition);
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
            const auto data = FreeParameterResampling::capture(array, clipStart, clipEnd);
            const auto sampled = FreeParameterResampling::resample(data, clipStart + deltaPosition);
            for (int index = 0; index < sampled.values.size(); ++index) {
                if (sampled.values.at(index).isValid()) {
                    target.insert(sampled.firstIndex + index,
                                  SingerCatalog::ParameterInfo::fromDspxModelValue(sampled.values.at(index).toInt()));
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
                    .value = SingerCatalog::ParameterInfo::fromDspxModelValue(node->y()),
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
                result[it.key()] = SingerCatalog::ParameterInfo::toDspxModelValue(it.value());
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
                node->setY(SingerCatalog::ParameterInfo::toDspxModelValue(point.value));
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

    class TransactionalModelStrategy : public Core::TransactionalStrategy {
    public:
        explicit TransactionalModelStrategy(dspx::Document *document, QObject *parent = nullptr)
            : Core::TransactionalStrategy(parent), m_document(document) {
        }

        bool beginTransaction() override {
            if (m_transaction || m_document->transaction())
                return false;
            m_transaction.emplace(m_document->engine()->beginTransaction());
            m_document->setTransaction(&*m_transaction);
            return true;
        }
        void abortTransaction() override {
            if (!m_transaction)
                return;
            m_document->setTransaction(nullptr);
            m_transaction->rollback();
            m_transaction.reset();
        }
        bool commitTransaction() override {
            if (!m_transaction)
                return false;
            const bool undoStepCreated = m_transaction->commit().createdUndoStep;
            m_document->setTransaction(nullptr);
            m_transaction.reset();
            return undoStepCreated;
        }
        void moveCurrentStepBy(int count) override {
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
    bool DocumentPrivate::emitOnChange(bool value, bool &cache, Signal signal) {
        if (cache == value)
            return false;
        cache = value;
        Q_EMIT (q_ptr->*signal)();
        return true;
    }

    void DocumentPrivate::initConnections() {
        updateEditScopeFocused();
        QObject::connect(selectionModel, &dspx::SelectionModel::hasSelectionChanged,
                         q_ptr, &Document::anyItemsSelectedChanged);
        QObject::connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, q_ptr, [this] {
            updateEditScopeFocused();
        });
        QObject::connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged,
                         q_ptr, &Document::pasteTargetsChanged);
        QObject::connect(selectionModel->trackSelectionModel(), &dspx::TrackSelectionModel::selectedItemsChanged,
                         q_ptr, &Document::pasteTargetsChanged);
        QObject::connect(selectionModel->clipSelectionModel(), &dspx::ClipSelectionModel::selectedItemsChanged,
                         q_ptr, &Document::pasteTargetsChanged);
        QObject::connect(selectionModel->clipSelectionModel(), &dspx::ClipSelectionModel::clipSequencesWithSelectedItemsChanged,
                         q_ptr, &Document::pasteTargetsChanged);
        QObject::connect(selectionModel->noteSelectionModel(), &dspx::NoteSelectionModel::noteSequenceWithSelectedItemsChanged,
                         q_ptr, &Document::pasteTargetsChanged);
        QObject::connect(selectionModel->anchorNodeSelectionModel(), &dspx::AnchorNodeSelectionModel::anchorNodeSequenceWithSelectedItemsChanged,
                         q_ptr, &Document::pasteTargetsChanged);
        QObject::connect(selectionModel->dynamicMixingAnchorSelectionModel(), &dspx::DynamicMixingAnchorSelectionModel::dynamicMixingAnchorSequenceWithSelectedItemsChanged,
                         q_ptr, &Document::pasteTargetsChanged);
        QObject::connect(selectionModel->freeParameterSelectionModel(), &dspx::FreeParameterSelectionModel::contextChanged,
                         q_ptr, &Document::pasteTargetsChanged);
    }

    bool DocumentPrivate::updateEditScopeFocused() {
        const bool value = selectionModel->selectionType() != dspx::SelectionModel::ST_None;
        return emitOnChange(value, editScopeFocused, &Document::editScopeFocusedChanged);
    }

    static dspx::Track *trackForClip(const dspx::Clip *clip) {
        auto *sequence = clip ? clip->clipSequence() : nullptr;
        return sequence ? sequence->track() : nullptr;
    }

    dspx::Track *DocumentPrivate::selectionTrack() const {
        dspx::Track *track = nullptr;
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Track: {
                const auto tracks = selectionModel->trackSelectionModel()->selectedItems();
                if (tracks.size() == 1)
                    track = tracks.first();
                break;
            }
            case dspx::SelectionModel::ST_Clip: {
                const auto sequences = selectionModel->clipSelectionModel()->clipSequencesWithSelectedItems();
                if (sequences.size() == 1)
                    track = sequences.first()->track();
                break;
            }
            default:
                track = trackForClip(selectionSingingClip());
                break;
        }
        return track && track->trackList() == model->tracks() ? track : nullptr;
    }

    std::optional<ClipboardData::Type> DocumentPrivate::selectionClipboardType() const {
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_FreeParameter:
                return ClipboardData::FreeParameter;
            case dspx::SelectionModel::ST_Tempo:
                return ClipboardData::Tempo;
            case dspx::SelectionModel::ST_Label:
                return ClipboardData::Label;
            case dspx::SelectionModel::ST_KeySignature:
                return ClipboardData::KeySignature;
            case dspx::SelectionModel::ST_Track:
                return ClipboardData::Track;
            case dspx::SelectionModel::ST_Clip:
                return ClipboardData::Clip;
            case dspx::SelectionModel::ST_Note:
                return selectionSingingClip() ? std::optional(ClipboardData::Note) : std::nullopt;
            case dspx::SelectionModel::ST_AnchorNode:
                return selectionSingingClip() ? std::optional(ClipboardData::AnchorNode) : std::nullopt;
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                return selectionSingingClip() ? std::optional(ClipboardData::DynamicMixingAnchor) : std::nullopt;
            case dspx::SelectionModel::ST_None:
                return std::nullopt;
        }
        return std::nullopt;
    }

    dspx::SingingClip *DocumentPrivate::selectionSingingClip() const {
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Clip: {
                const auto *clips = selectionModel->clipSelectionModel();
                if (clips->selectedSingingClipCount() != 1)
                    return nullptr;
                const auto items = clips->selectedItems();
                const auto it = std::ranges::find(items, dspx::Clip::Singing, &dspx::Clip::kind);
                return it != items.end() ? static_cast<dspx::SingingClip *>(*it) : nullptr;
            }
            case dspx::SelectionModel::ST_FreeParameter:
                return selectionModel->freeParameterSelectionModel()->singingClip();
            case dspx::SelectionModel::ST_Note: {
                auto *sequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
                return sequence ? sequence->singingClip() : nullptr;
            }
            case dspx::SelectionModel::ST_AnchorNode: {
                auto *sequence = selectionModel->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems();
                auto *map = sequence ? sequence->parameter()->parameterMap() : nullptr;
                return map ? map->singingClip() : nullptr;
            }
            case dspx::SelectionModel::ST_DynamicMixingAnchor: {
                auto *sequence = selectionModel->dynamicMixingAnchorSelectionModel()->dynamicMixingAnchorSequenceWithSelectedItems();
                return sequence ? sequence->sources()->singingClip() : nullptr;
            }
            default:
                return nullptr;
        }
    }

    std::optional<Document::PasteTarget> DocumentPrivate::resolvePasteTarget(ClipboardData::Type type, Document::PasteMode mode) const {
        Document::PasteTarget target(type, mode);
        switch (type) {
            case ClipboardData::Tempo:
            case ClipboardData::Label:
            case ClipboardData::KeySignature:
                return target;
            case ClipboardData::Clip:
                if (mode == Document::OriginalPosition)
                    return target;
                [[fallthrough]];
            case ClipboardData::Track:
                if (auto *track = selectionTrack())
                    target.m_track.emplace(track);
                else if (type == ClipboardData::Clip)
                    return std::nullopt;
                return target;
            case ClipboardData::Note:
            case ClipboardData::DynamicMixingAnchor:
            case ClipboardData::AnchorNode:
            case ClipboardData::FreeParameter:
                break;
        }

        auto *clip = selectionSingingClip();
        auto *track = trackForClip(clip);
        if (!track || track->trackList() != model->tracks())
            return std::nullopt;
        target.m_clip = clip;
        if (type == ClipboardData::Note)
            return target;
        if (type == ClipboardData::DynamicMixingAnchor) {
            auto *sources = clip->sources();
            if (!sources || sources->singers()->size() == 0)
                return std::nullopt;
            target.m_sources = sources;
            return target;
        }

        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_AnchorNode: {
                const auto *sequence = selectionModel->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems();
                const auto *parameters = clip->parameters();
                const auto keys = parameters->keys();
                const auto it = std::ranges::find_if(keys, [parameters, sequence](const QString &key) {
                    return parameters->item(key) == sequence->parameter();
                });
                if (it == keys.end())
                    return std::nullopt;
                target.m_parameterId = *it;
                target.m_parameterRole = sequence->role() == dspx::AnchorNodeSequence::Transform
                    ? dspx::FreeValueDataArray::Transform : dspx::FreeValueDataArray::Edited;
                break;
            }
            case dspx::SelectionModel::ST_FreeParameter: {
                const auto *selection = selectionModel->freeParameterSelectionModel();
                target.m_parameterId = selection->parameterId();
                target.m_parameterRole = selection->role();
                break;
            }
            default:
                return std::nullopt;
        }
        return target.m_parameterId.isEmpty() ? std::nullopt : std::optional(target);
    }

    std::optional<ClipboardData> DocumentPrivate::buildClipboardData(int playheadPosition) const {
        const auto type = selectionClipboardType();
        return type ? (this->*clipboardHandler(*type).copy)(playheadPosition) : std::nullopt;
    }

    std::optional<ClipboardData> DocumentPrivate::buildTempoClipboardData(int playheadPosition) const {
        return buildSequenceData(selectionModel->tempoSelectionModel()->selectedItems(), playheadPosition, &ClipboardData::setTempos);
    }

    std::optional<ClipboardData> DocumentPrivate::buildLabelClipboardData(int playheadPosition) const {
        return buildSequenceData(selectionModel->labelSelectionModel()->selectedItems(), playheadPosition, &ClipboardData::setLabels);
    }

    std::optional<ClipboardData> DocumentPrivate::buildKeySignatureClipboardData(int playheadPosition) const {
        return buildSequenceData(selectionModel->keySignatureSelectionModel()->selectedItems(), playheadPosition, &ClipboardData::setKeySignatures);
    }

    std::optional<ClipboardData> DocumentPrivate::buildNoteClipboardData(int playheadPosition) const {
        auto *clip = selectionSingingClip();
        return buildSequenceData(selectionModel->noteSelectionModel()->selectedItems(), playheadPosition - clip->start(), &ClipboardData::setNotes);
    }

    std::optional<ClipboardData> DocumentPrivate::buildDynamicMixingAnchorClipboardData(int playheadPosition) const {
        auto *clip = selectionSingingClip();
        return buildSequenceData(selectionModel->dynamicMixingAnchorSelectionModel()->selectedItems(), playheadPosition - clip->start(), &ClipboardData::setDynamicMixingAnchors);
    }

    std::optional<ClipboardData> DocumentPrivate::buildAnchorNodeClipboardData(int playheadPosition) const {
        auto *clip = selectionSingingClip();
        return buildSequenceData(selectionModel->anchorNodeSelectionModel()->selectedItems(), playheadPosition - clip->start(), &ClipboardData::setAnchorNodes);
    }

    std::optional<ClipboardData> DocumentPrivate::buildFreeParameterClipboardData(int playheadPosition) const {
        const auto *freeSelection = selectionModel->freeParameterSelectionModel();
        auto *clip = freeSelection->singingClip();
        auto *parameter = clip->parameters()->item(freeSelection->parameterId());
        const int start = freeSelection->start();
        const int end = freeSelection->end();
        FreeParameterClipboardData range {.length = end - start, .offset = -(start % dspx::FreeValueDataArray::step())};
        if (parameter) {
            auto *array = freeSelection->role() == dspx::FreeValueDataArray::Transform
                ? parameter->freeTransform() : parameter->freeEdited();
            range = FreeParameterResampling::capture(array, start, end);
        }
        ClipboardData data;
        data.setFreeParameter(range);
        data.setAbsolute(start);
        data.setPlayhead(playheadPosition - clip->start() - start);
        return data;
    }

    std::optional<ClipboardData> DocumentPrivate::buildTrackClipboardData(int) const {
        auto selectedItems = selectionModel->trackSelectionModel()->selectedItems();
        const auto orderedTracks = model->tracks()->items();
        std::sort(selectedItems.begin(), selectedItems.end(), [&orderedTracks](auto *left, auto *right) {
            return orderedTracks.indexOf(left) < orderedTracks.indexOf(right);
        });
        QList<opendspx::Track> tracks;
        tracks.reserve(selectedItems.size());
        for (const auto *track : std::as_const(selectedItems))
            tracks.append(track->toOpenDSPX());
        ClipboardData data;
        data.setTracks(tracks);
        return data;
    }

    std::optional<ClipboardData> DocumentPrivate::buildClipClipboardData(int playheadPosition) const {
        const auto selectedItems = selectionModel->clipSelectionModel()->selectedItems();
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
        ClipboardData data;
        data.setClips(clips);
        data.setAbsolute(absolute);
        data.setPlayhead(playheadPosition - absolute);
        data.setTrack(firstTrack);
        return data;
    }

    bool DocumentPrivate::pasteTempos(const ClipboardData &data, PasteContext &context) {
        return pasteSequenceData(data.tempos(), model->tempos(), model, context, [this] { return model->createTempo(); }, true);
    }

    bool DocumentPrivate::pasteLabels(const ClipboardData &data, PasteContext &context) {
        return pasteSequenceData(data.labels(), model->labels(), model, context, [this] { return model->createLabel(); }, false);
    }

    bool DocumentPrivate::pasteKeySignatures(const ClipboardData &data, PasteContext &context) {
        return pasteSequenceData(data.keySignatures(), model->keySignatures(), model, context, [this] { return model->createKeySignature(); }, true);
    }

    bool DocumentPrivate::pasteNotes(const ClipboardData &data, PasteContext &context) {
        auto *sequence = context.target.m_clip->notes();
        return pasteSequenceData(data.notes(), sequence, model, context, [this] { return model->createNote(); }, false);
    }

    bool DocumentPrivate::pasteDynamicMixingAnchors(const ClipboardData &data, PasteContext &context) {
        auto *sources = context.target.m_sources.data();
        const int singerCount = sources->singers()->size();
        auto anchors = data.dynamicMixingAnchors();
        for (auto &anchor : anchors)
            anchor.ratio.resize(static_cast<std::size_t>(singerCount - 1), 0.0);
        return pasteSequenceData(std::move(anchors), sources->dynamicMixingAnchors(), model, context, [this] { return model->createDynamicMixingAnchor(); }, true);
    }

    dspx::Parameter *DocumentPrivate::pasteParameter(const Document::PasteTarget &target, bool createIfMissing) {
        auto *parameters = target.m_clip->parameters();
        if (auto *parameter = parameters->item(target.m_parameterId))
            return parameter;
        if (!createIfMissing)
            return nullptr;
        auto *parameter = model->createParameter();
        if (!parameters->insertItem(target.m_parameterId, parameter)) {
            model->destroyItem(parameter);
            return nullptr;
        }
        return parameter;
    }

    bool DocumentPrivate::pasteAnchorNodes(const ClipboardData &data, PasteContext &context) {
        auto nodes = data.anchorNodes();
        if (nodes.isEmpty())
            return false;
        auto *parameter = pasteParameter(context.target, true);
        if (!parameter)
            return false;
        auto *sequence = context.target.m_parameterRole == dspx::FreeValueDataArray::Transform
            ? parameter->anchorTransform() : parameter->anchorEdited();
        return pasteSequenceData(std::move(nodes), sequence, model, context, [this] { return model->createAnchorNode(); }, true);
    }

    bool DocumentPrivate::pasteTracks(const ClipboardData &data, PasteContext &context) {
        auto *list = model->tracks();
        int insertionIndex = list->size();
        if (context.target.m_track)
            insertionIndex = list->items().indexOf(context.target.m_track->data()) + 1;
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

    bool DocumentPrivate::pasteClips(const ClipboardData &data, PasteContext &context) {
        const auto clips = data.clips();
        const bool hasClip = std::ranges::any_of(clips, [](const auto &trackClips) {
            return !trackClips.empty();
        });
        if (!hasClip)
            return false;
        auto *tracks = model->tracks();
        const int firstTrack = context.target.m_mode == Document::OriginalPosition
            ? std::max(0, data.track()) : tracks->items().indexOf(context.target.m_track->data());
        if (firstTrack < 0 || clips.size() > std::numeric_limits<int>::max() - firstTrack)
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

    bool DocumentPrivate::pasteFreeParameter(const ClipboardData &data, PasteContext &context) {
        const auto range = data.freeParameter();
        if (range.length > std::numeric_limits<int>::max() - context.position)
            return false;
        const auto sampled = FreeParameterResampling::resample(range, context.position);
        if (sampled.values.isEmpty())
            return false;
        const bool hasValues = std::ranges::any_of(sampled.values, [](const auto &value) { return value.isValid(); });
        auto *parameter = pasteParameter(context.target, hasValues);
        if (!parameter)
            return false;
        auto *array = context.target.m_parameterRole == dspx::FreeValueDataArray::Transform
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

    bool DocumentPrivate::canSafelyBounceIgnoringResampling() const {
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

    bool DocumentPrivate::bounceRequiresResampling() const {
        const auto trackBounceInfoMap = buildTrackBounceInfoMap(selectionModel);
        for (const auto &info : trackBounceInfoMap) {
            for (auto *clip : info.clips) {
                const int start = clip->clipStart();
                const int end = start + clip->clipLength();
                const int targetStart = clip->position() - info.bouncedClipTime.pos;
                for (const auto &id : clip->parameters()->keys()) {
                    auto *parameter = clip->parameters()->item(id);
                    if (FreeParameterResampling::requiresResampling(FreeParameterResampling::capture(parameter->freeTransform(), start, end), targetStart) ||
                        FreeParameterResampling::requiresResampling(FreeParameterResampling::capture(parameter->freeEdited(), start, end), targetStart)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool DocumentPrivate::deleteSelection() {
        if (!selectionModel->hasSelection())
            return false;

        int removedCount = 0;
        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_FreeParameter:
                return deleteFreeParameterSelection();
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
            qCInfo(lcDocument) << "Deleted selection" << selectionTypeName(selectionModel->selectionType()) << "count" << removedCount;
        else
            qCDebug(lcDocument) << "Delete selection produced no changes" << selectionTypeName(selectionModel->selectionType());

        return removedCount > 0;
    }

    int DocumentPrivate::deleteTempos() {
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

    int DocumentPrivate::deleteLabels() {
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

    int DocumentPrivate::deleteKeySignatures() {
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

    int DocumentPrivate::deleteTracks() {
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

    int DocumentPrivate::deleteClips() {
        int removedCount = 0;
        for (auto *item : selectionModel->clipSelectionModel()->selectedItems()) {
            if (item->clipSequence()->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DocumentPrivate::deleteNotes() {
        int removedCount = 0;
        for (auto *item : selectionModel->noteSelectionModel()->selectedItems()) {
            if (item->noteSequence()->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DocumentPrivate::deleteAnchorNodes() {
        auto *anchorSelectionModel = selectionModel->anchorNodeSelectionModel();
        auto *anchorSequence = anchorSelectionModel->anchorNodeSequenceWithSelectedItems();
        int removedCount = 0;
        for (auto *item : anchorSelectionModel->selectedItems()) {
            if (anchorSequence->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    int DocumentPrivate::deleteDynamicMixingAnchors() {
        auto *dynamicSelection = selectionModel->dynamicMixingAnchorSelectionModel();
        auto *sequence = dynamicSelection->dynamicMixingAnchorSequenceWithSelectedItems();
        int removedCount = 0;
        for (auto *item : dynamicSelection->selectedItems()) {
            if (sequence->removeItem(item)) {
                model->destroyItem(item);
                ++removedCount;
            }
        }
        return removedCount;
    }

    bool DocumentPrivate::deleteFreeParameterSelection() {
        const auto *freeSelection = selectionModel->freeParameterSelectionModel();
        auto *clip = freeSelection->singingClip();
        auto *parameter = clip->parameters()->item(freeSelection->parameterId());
        if (!parameter)
            return false;

        auto *values = freeSelection->role() == dspx::FreeValueDataArray::Transform
            ? parameter->freeTransform() : parameter->freeEdited();
        const int step = dspx::FreeValueDataArray::step();
        const int firstIndex = (static_cast<qint64>(freeSelection->start()) + step - 1) / step;
        const int endIndex = (static_cast<qint64>(freeSelection->end()) + step - 1) / step;
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

    void DocumentPrivate::selectAllTempos() {
        for (auto item : model->tempos()->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DocumentPrivate::selectAllLabels() {
        for (auto item : model->labels()->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DocumentPrivate::selectAllKeySignatures() {
        for (auto item : model->keySignatures()->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DocumentPrivate::selectAllTracks() {
        for (auto item : model->tracks()->items()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DocumentPrivate::selectAllClips() {
        for (auto track : model->tracks()->items()) {
            for (auto item : track->clips()->asRange()) {
                selectionModel->select(item, dspx::SelectionModel::Select);
            }
        }
    }

    void DocumentPrivate::selectAllNotes() {
        auto noteSequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
        if (!noteSequence)
            return;
        for (auto item : noteSequence->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DocumentPrivate::selectAllAnchorNodes() {
        auto *anchorSelectionModel = selectionModel->anchorNodeSelectionModel();
        auto *anchorSequence = anchorSelectionModel->anchorNodeSequenceWithSelectedItems();
        if (!anchorSequence)
            return;
        for (auto *item : anchorSequence->asRange()) {
            selectionModel->select(item, dspx::SelectionModel::Select);
        }
    }

    void DocumentPrivate::selectAllDynamicMixingAnchors() {
        auto *dynamicSelection = selectionModel->dynamicMixingAnchorSelectionModel();
        auto *sequence = dynamicSelection->dynamicMixingAnchorSequenceWithSelectedItems();
        if (!sequence)
            return;
        for (auto *item : sequence->asRange())
            selectionModel->select(item, dspx::SelectionModel::Select);
    }

    void DocumentPrivate::selectAllFreeParameter() {
        auto *clip = selectionModel->freeParameterSelectionModel()->singingClip();
        selectionModel->setFreeParameterRange(clip->clipStart(), clip->clipStart() + clip->clipLength());
    }

    Document::Document(QObject *parent) : QObject(parent), d_ptr(new DocumentPrivate) {
        Q_D(Document);
        d->q_ptr = this;
        d->dspxDocument = new dspx::Document(this);
        d->model = new dspx::Model(d->dspxDocument, this);
        d->selectionModel = new dspx::SelectionModel(d->model, this);
        auto transactionalStrategy = new TransactionalModelStrategy(d->dspxDocument);
        d->transactionController = new Core::TransactionController(transactionalStrategy, this);
        transactionalStrategy->setParent(d->transactionController);

        d->initConnections();
    }

    Document::~Document() = default;

    dspx::Model *Document::model() const {
        Q_D(const Document);
        return d->model;
    }

    dspx::SelectionModel *Document::selectionModel() const {
        Q_D(const Document);
        return d->selectionModel;
    }

    Core::TransactionController *Document::transactionController() const {
        Q_D(const Document);
        return d->transactionController;
    }

    bool Document::anyItemsSelected() {
        Q_D(Document);
        return d->selectionModel->hasSelection();
    }

    bool Document::isEditScopeFocused() {
        Q_D(Document);
        return d->editScopeFocused;
    }

    std::optional<ClipboardData> Document::copySelection(int playheadPosition) const {
        Q_D(const Document);
        return d->selectionModel->hasSelection() ? d->buildClipboardData(playheadPosition) : std::nullopt;
    }

    bool Document::PasteTarget::operator==(const PasteTarget &other) const = default;

    std::optional<Document::PasteTarget> Document::resolvePasteTarget(const QList<ClipboardData::Type> &types, PasteMode mode) const {
        Q_D(const Document);
        if (types.isEmpty())
            return std::nullopt;
        const auto preferredType = d->selectionClipboardType();
        if (preferredType && types.contains(*preferredType)) {
            if (auto target = d->resolvePasteTarget(*preferredType, mode))
                return target;
        }
        std::optional<PasteTarget> result;
        for (const auto type : types) {
            if (type == preferredType)
                continue;
            if (auto target = d->resolvePasteTarget(type, mode)) {
                if (result && result->type() != type)
                    return std::nullopt;
                result = std::move(target);
            }
        }
        return result;
    }

    bool Document::isPasteTargetValid(const PasteTarget &target) const {
        Q_D(const Document);
        const auto current = d->resolvePasteTarget(target.m_type, target.m_mode);
        return current && *current == target;
    }

    int Document::pastePosition(const ClipboardData &data, int playheadPosition, const PasteTarget &target) const {
        qint64 position = playheadPosition;
        if (target.m_clip)
            position -= target.m_clip->start();
        switch (target.m_mode) {
            case CurrentPosition:
                break;
            case RelativeToCopiedPlayhead:
                position -= data.playhead();
                break;
            case OriginalPosition:
                position = data.absolute();
                break;
        }
        return static_cast<int>(std::clamp<qint64>(position, 0, std::numeric_limits<int>::max()));
    }

    bool Document::pasteRequiresResampling(const ClipboardData &data, int position) const {
        return data.type() == ClipboardData::FreeParameter &&
               FreeParameterResampling::requiresResampling(data.freeParameter(), position);
    }

    std::optional<Document::PasteResult> Document::paste(const ClipboardData &data, int position, const PasteTarget &target) {
        Q_D(Document);
        if (target.type() != data.type() || !isPasteTargetValid(target)) {
            return std::nullopt;
        }
        const auto &handler = clipboardHandler(data.type());
        DocumentPrivate::PasteContext context{.target = target, .position = position};
        if (!(d->*handler.paste)(data, context)) {
            return std::nullopt;
        }
        PasteResult result{.items = std::move(context.items)};
        if (data.type() == ClipboardData::FreeParameter) {
            result.freeParameterRange = std::pair{context.rangeStart, context.rangeEnd};
        }
        return result;
    }

    bool Document::deleteSelection() {
        Q_D(Document);
        return d->deleteSelection();
    }

    void Document::selectItems(const QList<QObject *> &items, bool clearPrevious) {
        Q_D(Document);
        for (auto *item : items) {
            auto command = dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem;
            if (clearPrevious) {
                command |= dspx::SelectionModel::ClearPreviousSelection;
                clearPrevious = false;
            }
            d->selectionModel->select(item, command, dspx::SelectionModel::selectionTypeFromItem(item));
        }
    }

    void Document::selectPasteResult(const PasteResult &result, const PasteTarget &target) {
        Q_D(Document);
        if (result.freeParameterRange) {
            if (d->selectionModel->setFreeParameterContext(target.m_clip.data(), target.m_parameterId, target.m_parameterRole)) {
                d->selectionModel->setFreeParameterRange(result.freeParameterRange->first, result.freeParameterRange->second);
            }
        } else {
            selectItems(result.items, true);
        }
    }

    void Document::selectAll() {
        Q_D(Document);
        if (!isEditScopeFocused()) {
            return;
        }
        qCInfo(lcDocument) << "Select all" << selectionTypeName(d->selectionModel->selectionType());
        switch (d->selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_FreeParameter:
                d->selectAllFreeParameter();
                break;
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

    void Document::deselectAll() {
        Q_D(Document);
        if (!anyItemsSelected()) {
            return;
        }
        qCInfo(lcDocument) << "Deselect all" << selectionTypeName(d->selectionModel->selectionType());
        d->selectionModel->select(nullptr, dspx::SelectionModel::ClearPreviousSelection, d->selectionModel->selectionType());
    }

    QList<QObject *> Document::splitItems(int position) {
        Q_D(Document);
        QList<QObject *> inserted;
        const auto selectionType = d->selectionModel->selectionType();
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
                    const int previousLength = clip->clipLength();
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
                    if (!clipSequence->insertItem(newClip)) {
                        clip->setClipLength(previousLength);
                        d->model->destroyItem(newClip);
                        continue;
                    }

                    inserted.append(newClip);
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
                    const int previousLength = note->length();
                    note->setLength(newNoteLength);
                    data.length -= newNoteLength;
                    data.pos = position;

                    auto *newNote = d->model->createNote();
                    newNote->fromOpenDSPX(data);
                    newNote->setLyric("-");
                    auto *noteSequence = note->noteSequence();
                    if (!noteSequence->insertItem(newNote)) {
                        note->setLength(previousLength);
                        d->model->destroyItem(newNote);
                        continue;
                    }

                    inserted.append(newNote);
                }
                break;
            }
            default:
                break;
        }
        return inserted;
    }

    bool Document::isClipSingerLayoutIdentical() const {
        Q_D(const Document);
        const auto trackBounceInfoMap = buildTrackBounceInfoMap(d->selectionModel);
        if (trackBounceInfoMap.isEmpty())
            return false;
        return std::all_of(trackBounceInfoMap.cbegin(), trackBounceInfoMap.cend(), [](const BounceInfo &info) {
            return bounceInfoSingerLayoutIdentical(info);
        });
    }

    bool Document::canSafelyBounce() const {
        Q_D(const Document);
        return d->canSafelyBounceIgnoringResampling() && !d->bounceRequiresResampling();
    }

    std::optional<Document::BounceRisks> Document::bounceRisks() const {
        Q_D(const Document);
        const bool valid = d->selectionModel->selectionType() == dspx::SelectionModel::ST_Clip &&
                           d->selectionModel->clipSelectionModel()->selectedSingingClipCount() > 0;
        if (!valid) {
            return std::nullopt;
        }
        BounceRisks risks;
        risks.setFlag(BounceRisk::SingerLayoutMismatch, !isClipSingerLayoutIdentical());
        risks.setFlag(BounceRisk::DataLoss, !d->canSafelyBounceIgnoringResampling());
        risks.setFlag(BounceRisk::ParameterResampling, d->bounceRequiresResampling());
        return risks;
    }

    bool Document::bounceToClip() {
        Q_D(Document);
        const auto trackBounceInfoMap = buildTrackBounceInfoMap(d->selectionModel);
        if (trackBounceInfoMap.isEmpty()) {
            return false;
        }
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
    }

    void Document::loadModel(const opendspx::Model &model) {
        Q_D(Document);
        d->model->fromOpenDSPX(model);
    }

}

#include "moc_Document.cpp"
