// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DspxDocument.h"
#include "DspxDocument_p.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include <QHash>
#include <QLoggingCategory>
#include <QMap>
#include <QSet>

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
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
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
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingerList.h>
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

#include <transactional/TransactionController.h>
#include <transactional/TransactionalStrategy.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcDspxDocument, "diffscope.core.dspxdocument")

    static const char *clipboardTypeName(DspxClipboardData::Type type) {
        switch (type) {
            case DspxClipboardData::Tempo:
                return "Tempo";
            case DspxClipboardData::Label:
                return "Label";
            case DspxClipboardData::KeySignature:
                return "KeySignature";
            case DspxClipboardData::Track:
                return "Track";
            case DspxClipboardData::Clip:
                return "Clip";
            case DspxClipboardData::Note:
                return "Note";
            case DspxClipboardData::DynamicMixingAnchor:
                return "DynamicMixingAnchor";
        }
        return "Unknown";
    }

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
            const int step = dspx::FreeValueDataArray::step();
            const int firstIndex = clipStart / step;
            const int lastSegmentIndex = (clipEnd + step - 1) / step - 1;
            const auto values = array->slice(firstIndex, lastSegmentIndex - firstIndex + 2);
            for (int offset = 0; offset < values.size(); ++offset) {
                const int index = firstIndex + offset;
                const int position = index * step;
                if (offset + 1 < values.size() && values.at(offset).isValid() &&
                    values.at(offset + 1).isValid() &&
                    segmentIsClipped(position, position + step, clipStart, clipEnd)) {
                    return false;
                }
                if (!values.at(offset).isValid() || position < clipStart || position >= clipEnd)
                    continue;
                const int targetPosition = position + deltaPosition;
                if (targetPosition % step != 0 ||
                    !occupyPoint(occupied, parameterId, targetPosition)) {
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

        int ceilToFreeValueIndex(int position) {
            const int step = dspx::FreeValueDataArray::step();
            return (position + step - 1) / step;
        }

        std::optional<double> interpolatedFreeValue(const QList<QVariant> &values, int position) {
            const int step = dspx::FreeValueDataArray::step();
            const int leftIndex = position / step;
            if (leftIndex < 0 || leftIndex >= values.size())
                return std::nullopt;
            const auto &leftValue = values.at(leftIndex);
            if (!leftValue.isValid())
                return std::nullopt;
            if (position % step == 0)
                return ParameterInfo::fromDspxModelValue(leftValue.toInt());

            const int rightIndex = leftIndex + 1;
            if (rightIndex >= values.size() || !values.at(rightIndex).isValid())
                return std::nullopt;
            const double fraction = static_cast<double>(position - leftIndex * step) / step;
            const double left = ParameterInfo::fromDspxModelValue(leftValue.toInt());
            const double right = ParameterInfo::fromDspxModelValue(values.at(rightIndex).toInt());
            return left + (right - left) * fraction;
        }

        void mergeFreeValues(const dspx::FreeValueDataArray *array,
                             int clipStart,
                             int clipEnd,
                             int deltaPosition,
                             QMap<int, double> &target) {
            const int step = dspx::FreeValueDataArray::step();
            const auto values = array->items();
            if (deltaPosition % step == 0) {
                const int firstIndex = ceilToFreeValueIndex(clipStart);
                const int endIndex = ceilToFreeValueIndex(clipEnd);
                for (int index = firstIndex; index < endIndex && index < values.size(); ++index) {
                    if (values.at(index).isValid())
                        target.insert(index + deltaPosition / step,
                                      ParameterInfo::fromDspxModelValue(values.at(index).toInt()));
                }
                return;
            }

            const int targetStart = clipStart + deltaPosition;
            const int targetEnd = clipEnd + deltaPosition;
            const int firstTargetIndex = ceilToFreeValueIndex(targetStart);
            const int endTargetIndex = ceilToFreeValueIndex(targetEnd);
            for (int targetIndex = firstTargetIndex; targetIndex < endTargetIndex; ++targetIndex) {
                const int sourcePosition = targetIndex * step - deltaPosition;
                if (const auto value = interpolatedFreeValue(values, sourcePosition))
                    target.insert(targetIndex, *value);
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

        void clearOriginalParameters(const BounceInfo &info) {
            for (auto *clip : info.clips) {
                for (const auto &parameterId : clip->parameters()->keys()) {
                    if (auto *parameter = clip->parameters()->item(parameterId))
                        parameter->original()->splice(0, parameter->original()->size(), {});
                }
            }
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

    std::optional<DspxClipboardData::Type> DspxDocumentPrivate::currentClipboardType() const {
        if (freeParameterSelectionModel && freeParameterSelectionModel->isActive()) {
            // TODO(parameter clipboard format): return the free-parameter clipboard type.
            return std::nullopt;
        }
        if (!selectionModel)
            return std::nullopt;

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
                return selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems()
                           ? std::optional(DspxClipboardData::Note)
                           : std::nullopt;
            case dspx::SelectionModel::ST_AnchorNode:
                // TODO(parameter clipboard format): return the anchor-node clipboard type.
                return std::nullopt;
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                return selectionModel->dynamicMixingAnchorSelectionModel()
                               ->dynamicMixingAnchorSequenceWithSelectedItems()
                           ? std::optional(DspxClipboardData::DynamicMixingAnchor)
                           : std::nullopt;
            case dspx::SelectionModel::ST_None:
            default:
                return std::nullopt;
        }
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

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildClipboardData(int playheadPosition) const {
        if (freeParameterSelectionModel && freeParameterSelectionModel->isActive()) {
            copyFreeParameterSelection(playheadPosition);
            return std::nullopt;
        }
        if (!selectionModel)
            return std::nullopt;

        switch (selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Tempo:
                return buildTempoClipboardData(playheadPosition);
            case dspx::SelectionModel::ST_Label:
                return buildLabelClipboardData(playheadPosition);
            case dspx::SelectionModel::ST_KeySignature:
                return buildKeySignatureClipboardData(playheadPosition);
            case dspx::SelectionModel::ST_Track:
                return buildTrackClipboardData();
            case dspx::SelectionModel::ST_Clip:
                return buildClipClipboardData(playheadPosition);
            case dspx::SelectionModel::ST_Note:
                return buildNoteClipboardData(playheadPosition);
            case dspx::SelectionModel::ST_AnchorNode:
                copyAnchorNodeSelection(playheadPosition);
                return std::nullopt;
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                return buildDynamicMixingAnchorClipboardData(playheadPosition);
            case dspx::SelectionModel::ST_None:
            default:
                return std::nullopt;
        }
    }

    bool DspxDocumentPrivate::copyAnchorNodeSelection(int) const {
        // TODO(parameter clipboard format): serialize the selected anchor nodes.
        return false;
    }

    bool DspxDocumentPrivate::copyFreeParameterSelection(int) const {
        // TODO(parameter clipboard format): serialize the selected free-parameter range.
        return false;
    }

    bool DspxDocumentPrivate::pasteAnchorNodeSelection(int) {
        // TODO(parameter clipboard format): paste anchor nodes into the active parameter.
        return false;
    }

    bool DspxDocumentPrivate::pasteFreeParameterSelection(int) {
        // TODO(parameter clipboard format): paste free values into the active parameter range.
        return false;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildTempoClipboardData(int playheadPosition) const {
        if (!model || !selectionModel)
            return std::nullopt;

        const auto selectedItems = selectionModel->tempoSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;

        QList<opendspx::Tempo> tempos;
        tempos.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            tempos.append(item->toOpenDSPX());
        }
        if (tempos.isEmpty())
            return std::nullopt;

        std::sort(tempos.begin(), tempos.end(), [](const opendspx::Tempo &lhs, const opendspx::Tempo &rhs) {
            return lhs.pos < rhs.pos;
        });

        const int absolute = tempos.first().pos;
        for (auto &tempo : tempos)
            tempo.pos -= absolute;

        DspxClipboardData data;
        data.setTempos(tempos);
        data.setAbsolute(absolute);
        data.setPlayhead(playheadPosition - absolute);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildLabelClipboardData(int playheadPosition) const {
        if (!model || !selectionModel)
            return std::nullopt;

        const auto selectedItems = selectionModel->labelSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;

        QList<opendspx::Label> labels;
        labels.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            labels.append(item->toOpenDSPX());
        }
        if (labels.isEmpty())
            return std::nullopt;

        std::sort(labels.begin(), labels.end(), [](const opendspx::Label &lhs, const opendspx::Label &rhs) {
            return lhs.pos < rhs.pos;
        });

        const int absolute = labels.first().pos;
        for (auto &label : labels)
            label.pos -= absolute;

        DspxClipboardData data;
        data.setLabels(labels);
        data.setAbsolute(absolute);
        data.setPlayhead(playheadPosition - absolute);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildDynamicMixingAnchorClipboardData(
        int playheadPosition) const {
        if (!model || !selectionModel)
            return std::nullopt;

        auto *dynamicSelection = selectionModel->dynamicMixingAnchorSelectionModel();
        auto *sequence = dynamicSelection->dynamicMixingAnchorSequenceWithSelectedItems();
        const auto selectedItems = dynamicSelection->selectedItems();
        if (!sequence || selectedItems.isEmpty())
            return std::nullopt;

        QList<opendspx::DynamicMixingAnchor> anchors;
        anchors.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            if (item->dynamicMixingAnchorSequence() == sequence)
                anchors.append(item->toOpenDSPX());
        }
        if (anchors.isEmpty())
            return std::nullopt;

        std::sort(anchors.begin(), anchors.end(), [](const auto &left, const auto &right) {
            return left.pos < right.pos;
        });
        const int absolute = anchors.first().pos;
        for (auto &anchor : anchors)
            anchor.pos -= absolute;

        auto *clip = sequence->sources()->singingClip();
        const int localPlayhead = clip
                                      ? playheadPosition - clip->position() + clip->clipStart()
                                      : playheadPosition;
        DspxClipboardData data;
        data.setDynamicMixingAnchors(anchors);
        data.setAbsolute(absolute);
        data.setPlayhead(localPlayhead - absolute);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildKeySignatureClipboardData(int playheadPosition) const {
        if (!model || !selectionModel)
            return std::nullopt;

        const auto selectedItems = selectionModel->keySignatureSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;

        QList<stdc::JsonValue> keySignatures;
        keySignatures.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            keySignatures.append(item->toOpenDSPX());
        }
        if (keySignatures.isEmpty())
            return std::nullopt;

        std::sort(keySignatures.begin(), keySignatures.end(), [](const stdc::JsonValue &lhs, const stdc::JsonValue &rhs) {
            return lhs["pos"].toInt() < rhs["pos"].toInt();
        });

        const int absolute = static_cast<int>(keySignatures.first()["pos"].toInt());
        for (auto &keySignature : keySignatures) {
            stdc::JsonObject object = keySignature.toObject();
            object["pos"] = static_cast<int>(object["pos"].toInt()) - absolute;
            keySignature = std::move(object);
        }

        DspxClipboardData data;
        data.setKeySignatures(keySignatures);
        data.setAbsolute(absolute);
        data.setPlayhead(playheadPosition - absolute);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildTrackClipboardData() const {
        if (!model || !selectionModel)
            return std::nullopt;

        auto selectedItems = selectionModel->trackSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;

        const auto orderedTracks = model->tracks()->items();
        std::sort(selectedItems.begin(), selectedItems.end(), [&orderedTracks](dspx::Track *lhs, dspx::Track *rhs) {
            return orderedTracks.indexOf(lhs) < orderedTracks.indexOf(rhs);
        });

        QList<opendspx::Track> tracks;
        tracks.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            tracks.append(item->toOpenDSPX());
        }

        if (tracks.isEmpty())
            return std::nullopt;

        DspxClipboardData data;
        data.setTracks(tracks);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildClipClipboardData(int playheadPosition) const {
        if (!model || !selectionModel)
            return std::nullopt;

        const auto selectedItems = selectionModel->clipSelectionModel()->selectedItems();
        if (selectedItems.isEmpty())
            return std::nullopt;

        struct SelectedClip {
            dspx::Clip *clip;
            int trackIndex;
        };

        const auto tracks = model->tracks()->items();
        QList<SelectedClip> orderedClips;
        orderedClips.reserve(selectedItems.size());
        int firstTrackIndex = static_cast<int>(tracks.size());
        int lastTrackIndex = -1;
        int absolute = std::numeric_limits<int>::max();
        for (auto *clip : selectedItems) {
            auto *clipSequence = clip ? clip->clipSequence() : nullptr;
            auto *track = clipSequence ? clipSequence->track() : nullptr;
            const int trackIndex = tracks.indexOf(track);
            if (!clip || trackIndex < 0)
                continue;
            orderedClips.append({clip, trackIndex});
            firstTrackIndex = std::min(firstTrackIndex, trackIndex);
            lastTrackIndex = std::max(lastTrackIndex, trackIndex);
            absolute = std::min(absolute, clip->position());
        }
        if (orderedClips.isEmpty())
            return std::nullopt;

        std::sort(orderedClips.begin(), orderedClips.end(), [](const SelectedClip &left, const SelectedClip &right) {
            if (left.trackIndex != right.trackIndex)
                return left.trackIndex < right.trackIndex;
            if (left.clip->position() != right.clip->position())
                return left.clip->position() < right.clip->position();
            return left.clip->clipLength() < right.clip->clipLength();
        });

        QList<std::vector<opendspx::ClipRef>> clips;
        clips.resize(lastTrackIndex - firstTrackIndex + 1);
        int copiedCount = 0;
        for (const auto &selectedClip : std::as_const(orderedClips)) {
            auto clip = selectedClip.clip->toOpenDSPX();
            if (!clip)
                continue;
            clip->time.pos -= absolute;
            clips[selectedClip.trackIndex - firstTrackIndex].push_back(std::move(clip));
            ++copiedCount;
        }
        if (copiedCount == 0)
            return std::nullopt;

        DspxClipboardData data;
        data.setClips(clips);
        data.setPlayhead(playheadPosition - absolute);
        data.setAbsolute(absolute);
        data.setTrack(firstTrackIndex);
        return data;
    }

    std::optional<DspxClipboardData> DspxDocumentPrivate::buildNoteClipboardData(int playheadPosition) const {
        if (!model || !selectionModel)
            return std::nullopt;

        auto *noteSelection = selectionModel->noteSelectionModel();
        auto *noteSequence = noteSelection->noteSequenceWithSelectedItems();
        auto *clip = noteSequence ? noteSequence->singingClip() : nullptr;
        const auto selectedItems = noteSelection->selectedItems();
        if (!clip || selectedItems.isEmpty())
            return std::nullopt;

        QList<opendspx::Note> notes;
        notes.reserve(selectedItems.size());
        for (const auto *item : selectedItems) {
            if (item && item->noteSequence() == noteSequence)
                notes.append(item->toOpenDSPX());
        }
        if (notes.isEmpty())
            return std::nullopt;

        std::sort(notes.begin(), notes.end(), [](const opendspx::Note &left, const opendspx::Note &right) {
            if (left.pos != right.pos)
                return left.pos < right.pos;
            if (left.keyNum != right.keyNum)
                return left.keyNum < right.keyNum;
            return left.length < right.length;
        });

        const int absolute = notes.first().pos;
        for (auto &note : notes)
            note.pos -= absolute;

        const int localPlayhead = playheadPosition - clip->position() + clip->clipStart();
        DspxClipboardData data;
        data.setNotes(notes);
        data.setPlayhead(localPlayhead - absolute);
        data.setAbsolute(absolute);
        return data;
    }

    bool DspxDocumentPrivate::pasteClipboardData(const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems) {
        switch (data.type()) {
            case DspxClipboardData::Tempo:
                return pasteTempos(data.tempos(), data, playheadPosition, pastedItems);
            case DspxClipboardData::Label:
                return pasteLabels(data.labels(), data, playheadPosition, pastedItems);
            case DspxClipboardData::KeySignature:
                return pasteKeySignatures(data.keySignatures(), data, playheadPosition, pastedItems);
            case DspxClipboardData::Track:
                return pasteTracks(data.tracks(), pastedItems);
            case DspxClipboardData::Clip:
                return pasteClips(data.clips(), data, playheadPosition, pastedItems);
            case DspxClipboardData::Note:
                return pasteNotes(data.notes(), data, playheadPosition, pastedItems);
            case DspxClipboardData::DynamicMixingAnchor:
                return pasteDynamicMixingAnchors(data.dynamicMixingAnchors(), data,
                                                 playheadPosition, pastedItems);
        }
        return false;
    }

    bool DspxDocumentPrivate::pasteTempos(const QList<opendspx::Tempo> &tempos, const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems) {
        if (!model || tempos.isEmpty())
            return false;

        enum class PasteMode {
            CurrentPosition,
            RelativeToCopiedPlayhead,
            OriginalPosition,
        };

        // TODO allow selecting paste mode from user settings
        const auto pasteMode = PasteMode::CurrentPosition;

        const auto baseOffset = [pasteMode, &data, playheadPosition] {
            switch (pasteMode) {
                case PasteMode::CurrentPosition:
                    return playheadPosition;
                case PasteMode::RelativeToCopiedPlayhead:
                    return playheadPosition - data.playhead();
                case PasteMode::OriginalPosition:
                    return data.absolute();
            }
            return playheadPosition;
        }();

        QList<opendspx::Tempo> adjusted = tempos;
        int minPos = adjusted.isEmpty() ? 0 : std::numeric_limits<int>::max();
        for (auto &tempo : adjusted) {
            tempo.pos += baseOffset;
            minPos = std::min(minPos, tempo.pos);
        }
        if (minPos < 0) {
            const int shift = -minPos;
            for (auto &tempo : adjusted)
                tempo.pos += shift;
        }

        std::sort(adjusted.begin(), adjusted.end(), [](const opendspx::Tempo &lhs, const opendspx::Tempo &rhs) {
            return lhs.pos < rhs.pos;
        });

        bool inserted = false;
        int removedOverlaps = 0;
        auto tempoSequence = model->tempos();
        for (const auto &tempoData : adjusted) {
            const auto overlappingItems = tempoSequence->slice(tempoData.pos, 1);
            for (auto *overlappingItem : overlappingItems) {
                tempoSequence->removeItem(overlappingItem);
                model->destroyItem(overlappingItem);
                ++removedOverlaps;
            }

            auto *tempo = model->createTempo();
            tempo->fromOpenDSPX(tempoData);
            if (!tempoSequence->insertItem(tempo)) {
                model->destroyItem(tempo);
                continue;
            }

            inserted = true;
            pastedItems.append(tempo);
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted tempos" << adjusted.size() << "removed overlaps" << removedOverlaps;
        else
            qCDebug(lcDspxDocument) << "No tempo pasted";

        return inserted;
    }

    bool DspxDocumentPrivate::pasteLabels(const QList<opendspx::Label> &labels, const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems) {
        if (!model || labels.isEmpty())
            return false;

        enum class PasteMode {
            CurrentPosition,
            RelativeToCopiedPlayhead,
            OriginalPosition,
        };

        // TODO allow selecting paste mode from user settings
        const auto pasteMode = PasteMode::CurrentPosition;

        const auto baseOffset = [pasteMode, &data, playheadPosition] {
            switch (pasteMode) {
                case PasteMode::CurrentPosition:
                    return playheadPosition;
                case PasteMode::RelativeToCopiedPlayhead:
                    return playheadPosition - data.playhead();
                case PasteMode::OriginalPosition:
                    return data.absolute();
            }
            return playheadPosition;
        }();

        QList<opendspx::Label> adjusted = labels;
        int minPos = adjusted.isEmpty() ? 0 : std::numeric_limits<int>::max();
        for (auto &label : adjusted) {
            label.pos += baseOffset;
            minPos = std::min(minPos, label.pos);
        }
        if (minPos < 0) {
            const int shift = -minPos;
            for (auto &label : adjusted)
                label.pos += shift;
        }

        std::sort(adjusted.begin(), adjusted.end(), [](const opendspx::Label &lhs, const opendspx::Label &rhs) {
            return lhs.pos < rhs.pos;
        });

        bool inserted = false;
        auto labelSequence = model->labels();
        for (const auto &labelData : adjusted) {
            auto *label = model->createLabel();
            label->fromOpenDSPX(labelData);
            if (!labelSequence->insertItem(label)) {
                model->destroyItem(label);
                continue;
            }
            inserted = true;
            pastedItems.append(label);
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted labels" << adjusted.size();
        else
            qCDebug(lcDspxDocument) << "No label pasted";

        return inserted;
    }

    bool DspxDocumentPrivate::pasteDynamicMixingAnchors(
        const QList<opendspx::DynamicMixingAnchor> &anchors, const DspxClipboardData &data,
        int playheadPosition, QList<QObject *> &pastedItems) {
        Q_UNUSED(data)
        if (!model || !selectionModel || anchors.isEmpty())
            return false;

        auto *sequence = selectionModel->dynamicMixingAnchorSelectionModel()
                             ->dynamicMixingAnchorSequenceWithSelectedItems();
        if (!sequence || !sequence->sources() || !sequence->sources()->singingClip())
            return false;
        const int singerCount = sequence->sources()->singers()->size();
        if (singerCount <= 0)
            return false;

        auto *clip = sequence->sources()->singingClip();
        const int localPlayhead = playheadPosition - clip->position() + clip->clipStart();
        QList<opendspx::DynamicMixingAnchor> adjusted = anchors;
        int minimumPosition = std::numeric_limits<int>::max();
        for (auto &anchor : adjusted) {
            anchor.pos += localPlayhead;
            minimumPosition = std::min(minimumPosition, anchor.pos);
            anchor.ratio.resize(static_cast<std::size_t>(std::max(0, singerCount - 1)), 0.0);
        }
        if (minimumPosition < 0) {
            for (auto &anchor : adjusted)
                anchor.pos -= minimumPosition;
        }
        std::sort(adjusted.begin(), adjusted.end(), [](const auto &left, const auto &right) {
            return left.pos < right.pos;
        });

        QList<opendspx::DynamicMixingAnchor> uniqueAnchors;
        uniqueAnchors.reserve(adjusted.size());
        for (const auto &anchor : std::as_const(adjusted)) {
            if (!uniqueAnchors.isEmpty() && uniqueAnchors.last().pos == anchor.pos)
                uniqueAnchors.last() = anchor;
            else
                uniqueAnchors.append(anchor);
        }

        bool inserted = false;
        int removedOverlaps = 0;
        for (const auto &anchorData : std::as_const(uniqueAnchors)) {
            for (auto *overlap : sequence->slice(anchorData.pos, 1)) {
                sequence->removeItem(overlap);
                model->destroyItem(overlap);
                ++removedOverlaps;
            }
            auto *anchor = model->createDynamicMixingAnchor();
            anchor->fromOpenDSPX(anchorData);
            if (!sequence->insertItem(anchor)) {
                model->destroyItem(anchor);
                continue;
            }
            inserted = true;
            pastedItems.append(anchor);
        }

        if (inserted) {
            qCInfo(lcDspxDocument) << "Pasted voice blending anchors" << uniqueAnchors.size()
                                   << "removed overlaps" << removedOverlaps;
        } else {
            qCDebug(lcDspxDocument) << "No voice blending anchor pasted";
        }
        return inserted;
    }

    bool DspxDocumentPrivate::pasteKeySignatures(const QList<stdc::JsonValue> &keySignatures, const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems) {
        if (!model || keySignatures.isEmpty())
            return false;

        enum class PasteMode {
            CurrentPosition,
            RelativeToCopiedPlayhead,
            OriginalPosition,
        };

        // TODO allow selecting paste mode from user settings
        const auto pasteMode = PasteMode::CurrentPosition;

        const auto baseOffset = [pasteMode, &data, playheadPosition] {
            switch (pasteMode) {
                case PasteMode::CurrentPosition:
                    return playheadPosition;
                case PasteMode::RelativeToCopiedPlayhead:
                    return playheadPosition - data.playhead();
                case PasteMode::OriginalPosition:
                    return data.absolute();
            }
            return playheadPosition;
        }();

        QList<stdc::JsonValue> adjusted = keySignatures;
        int minPos = adjusted.isEmpty() ? 0 : std::numeric_limits<int>::max();
        for (auto &keySignature : adjusted) {
            stdc::JsonObject object = keySignature.toObject();
            const int pos = keySignature.isObject() && keySignature["pos"].isNumber() ? static_cast<int>(keySignature["pos"].toInt()) + baseOffset : baseOffset;
            object["pos"] = pos;
            keySignature = std::move(object);
            minPos = std::min(minPos, pos);
        }
        if (minPos < 0) {
            const int shift = -minPos;
            for (auto &keySignature : adjusted) {
                stdc::JsonObject object = keySignature.toObject();
                object["pos"] = static_cast<int>(object["pos"].toInt()) + shift;
                keySignature = std::move(object);
            }
        }

        std::sort(adjusted.begin(), adjusted.end(), [](const stdc::JsonValue &lhs, const stdc::JsonValue &rhs) {
            return lhs["pos"].toInt() < rhs["pos"].toInt();
        });

        bool inserted = false;
        int removedOverlaps = 0;
        auto keySignatureSequence = model->keySignatures();
        for (const auto &keySignatureData : adjusted) {
            const auto pos = static_cast<int>(keySignatureData["pos"].toInt());
            const auto overlappingItems = keySignatureSequence->slice(pos, 1);
            for (auto *overlappingItem : overlappingItems) {
                keySignatureSequence->removeItem(overlappingItem);
                model->destroyItem(overlappingItem);
                ++removedOverlaps;
            }

            auto *keySignature = model->createKeySignature();
            keySignature->fromOpenDSPX(keySignatureData);
            if (!keySignatureSequence->insertItem(keySignature)) {
                model->destroyItem(keySignature);
                continue;
            }

            inserted = true;
            pastedItems.append(keySignature);
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted key signatures" << adjusted.size() << "removed overlaps" << removedOverlaps;
        else
            qCDebug(lcDspxDocument) << "No key signature pasted";

        return inserted;
    }

    bool DspxDocumentPrivate::pasteTracks(const QList<opendspx::Track> &tracks, QList<QObject *> &pastedItems) {
        if (!model || tracks.isEmpty())
            return false;

        bool inserted = false;
        auto *trackList = model->tracks();
        auto insertionIndex = trackList->size();
        do {
            auto currentTrack = qobject_cast<dspx::Track *>(selectionModel->currentItem());
            if (!currentTrack) {
                break;
            }
            auto index = trackList->items().indexOf(currentTrack);
            if (index == -1) {
                break;
            }
            insertionIndex = index + 1;
        } while (false);
        for (const auto &trackData : tracks) {
            auto *track = model->createTrack();
            track->fromOpenDSPX(trackData);
            if (!trackList->insertItem(insertionIndex, track)) {
                model->destroyItem(track);
                continue;
            }
            insertionIndex++;
            inserted = true;
            pastedItems.append(track);
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted tracks" << tracks.size();
        else
            qCDebug(lcDspxDocument) << "No track pasted";

        return inserted;
    }

    bool DspxDocumentPrivate::pasteClips(const QList<std::vector<opendspx::ClipRef>> &clips,
                                         const DspxClipboardData &data, int playheadPosition,
                                         QList<QObject *> &pastedItems) {
        if (!model || !selectionModel || clips.isEmpty())
            return false;

        enum class PasteMode {
            CurrentPositionAndCurrentTrack,
            RelativeToCopiedPlayheadAndCurrentTrack,
            OriginalPositionAndOriginalTrack,
        };

        // TODO allow selecting paste mode from user settings
        const auto pasteMode = PasteMode::CurrentPositionAndCurrentTrack;

        const int baseOffset = [pasteMode, &data, playheadPosition] {
            switch (pasteMode) {
                case PasteMode::CurrentPositionAndCurrentTrack:
                    return playheadPosition;
                case PasteMode::RelativeToCopiedPlayheadAndCurrentTrack:
                    return playheadPosition - data.playhead();
                case PasteMode::OriginalPositionAndOriginalTrack:
                    return data.absolute();
            }
            return playheadPosition;
        }();

        bool hasValidClip = false;
        int minimumPosition = std::numeric_limits<int>::max();
        for (const auto &trackClips : clips) {
            for (const auto &clip : trackClips) {
                if (!clip)
                    continue;
                hasValidClip = true;
                minimumPosition = std::min(minimumPosition, clip->time.pos + baseOffset);
            }
        }
        if (!hasValidClip)
            return false;
        const int nonNegativeShift = minimumPosition < 0 ? -minimumPosition : 0;

        auto *trackList = model->tracks();
        int targetTrackIndex = pasteMode == PasteMode::OriginalPositionAndOriginalTrack
                                   ? data.track()
                                   : currentTrackIndex();
        targetTrackIndex = std::max(targetTrackIndex, 0);

        const int trackSpan = static_cast<int>(clips.size());
        const int requiredTrackCount = targetTrackIndex + trackSpan;
        while (trackList->size() < requiredTrackCount) {
            auto *track = model->createTrack();
            track->fromOpenDSPX(opendspx::Track {});
            if (!trackList->insertItem(trackList->size(), track)) {
                model->destroyItem(track);
                return false;
            }
        }

        bool inserted = false;
        for (int relativeTrackIndex = 0; relativeTrackIndex < trackSpan; ++relativeTrackIndex) {
            auto *targetTrack = trackList->item(targetTrackIndex + relativeTrackIndex);
            auto *targetSequence = targetTrack ? targetTrack->clips() : nullptr;
            if (!targetSequence)
                continue;

            for (const auto &clipData : clips.at(relativeTrackIndex)) {
                if (!clipData)
                    continue;

                dspx::Clip *clip = nullptr;
                switch (clipData->type) {
                    case opendspx::Clip::Type::Audio:
                        clip = model->createAudioClip();
                        break;
                    case opendspx::Clip::Type::Singing:
                        clip = model->createSingingClip();
                        break;
                }
                if (!clip)
                    continue;

                clip->fromOpenDSPX(clipData);
                clip->setPosition(clipData->time.pos + baseOffset + nonNegativeShift);
                if (!targetSequence->insertItem(clip)) {
                    model->destroyItem(clip);
                    continue;
                }
                inserted = true;
                pastedItems.append(clip);
            }
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted clips" << pastedItems.size();
        else
            qCDebug(lcDspxDocument) << "No clip pasted";
        return inserted;
    }

    bool DspxDocumentPrivate::pasteNotes(const QList<opendspx::Note> &notes,
                                         const DspxClipboardData &data, int playheadPosition,
                                         QList<QObject *> &pastedItems) {
        if (!model || !selectionModel || notes.isEmpty())
            return false;

        auto *noteSequence = selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
        auto *clip = noteSequence ? noteSequence->singingClip() : nullptr;
        if (!clip)
            return false;

        enum class PasteMode {
            CurrentPosition,
            RelativeToCopiedPlayhead,
            OriginalPosition,
        };

        // TODO allow selecting paste mode from user settings
        const auto pasteMode = PasteMode::CurrentPosition;
        const int localPlayhead = playheadPosition - clip->position() + clip->clipStart();
        const int baseOffset = [pasteMode, &data, localPlayhead] {
            switch (pasteMode) {
                case PasteMode::CurrentPosition:
                    return localPlayhead;
                case PasteMode::RelativeToCopiedPlayhead:
                    return localPlayhead - data.playhead();
                case PasteMode::OriginalPosition:
                    return data.absolute();
            }
            return localPlayhead;
        }();

        QList<opendspx::Note> adjusted = notes;
        int minimumPosition = std::numeric_limits<int>::max();
        for (auto &note : adjusted) {
            note.pos += baseOffset;
            minimumPosition = std::min(minimumPosition, note.pos);
        }
        if (minimumPosition < 0) {
            for (auto &note : adjusted)
                note.pos -= minimumPosition;
        }
        std::sort(adjusted.begin(), adjusted.end(), [](const opendspx::Note &left, const opendspx::Note &right) {
            if (left.pos != right.pos)
                return left.pos < right.pos;
            if (left.keyNum != right.keyNum)
                return left.keyNum < right.keyNum;
            return left.length < right.length;
        });

        bool inserted = false;
        for (const auto &noteData : std::as_const(adjusted)) {
            auto *note = model->createNote();
            note->fromOpenDSPX(noteData);
            if (!noteSequence->insertItem(note)) {
                model->destroyItem(note);
                continue;
            }
            inserted = true;
            pastedItems.append(note);
        }

        if (inserted)
            qCInfo(lcDspxDocument) << "Pasted notes" << adjusted.size();
        else
            qCDebug(lcDspxDocument) << "No note pasted";
        return inserted;
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
        if (!anyItemsSelected())
            return;
        Q_D(DspxDocument);
        if ((d->freeParameterSelectionModel && d->freeParameterSelectionModel->isActive()) ||
            (d->selectionModel && d->selectionModel->selectionType() == dspx::SelectionModel::ST_AnchorNode)) {
            // Clipboard serialization is deliberately deferred. Never delete when copying did not succeed.
            copySelection(playheadPosition);
            return;
        }
        copySelection(playheadPosition);
        deleteSelection();
    }

    void DspxDocument::copySelection(int playheadPosition) {
        Q_D(DspxDocument);
        if (!anyItemsSelected())
            return;
        qCInfo(lcDspxDocument) << "Copy selection";
        const auto clipboardData = d->buildClipboardData(playheadPosition);
        if (!clipboardData.has_value())
            return;

        if (auto *clipboard = DspxClipboard::instance())
            clipboard->copy({clipboardData.value()}, nullptr);

        auto copiedType = clipboardData->type();
        int copiedCount = 0;
        switch (copiedType) {
            case DspxClipboardData::Tempo:
                copiedCount = clipboardData->tempos().size();
                break;
            case DspxClipboardData::Label:
                copiedCount = clipboardData->labels().size();
                break;
            case DspxClipboardData::KeySignature:
                copiedCount = clipboardData->keySignatures().size();
                break;
            case DspxClipboardData::Track:
                copiedCount = clipboardData->tracks().size();
                break;
            case DspxClipboardData::Clip:
                for (const auto &clips : clipboardData->clips())
                    copiedCount += static_cast<int>(clips.size());
                break;
            case DspxClipboardData::Note:
                copiedCount = clipboardData->notes().size();
                break;
            case DspxClipboardData::DynamicMixingAnchor:
                copiedCount = clipboardData->dynamicMixingAnchors().size();
                break;
        }
        qCInfo(lcDspxDocument) << "Copied selection" << clipboardTypeName(copiedType) << "count" << copiedCount;

        d->updatePasteAvailable();
    }

    void DspxDocument::paste(int playheadPosition) {
        Q_D(DspxDocument);
        if (d->freeParameterSelectionModel && d->freeParameterSelectionModel->isActive()) {
            d->pasteFreeParameterSelection(playheadPosition);
            return;
        }
        if (d->selectionModel && d->selectionModel->selectionType() == dspx::SelectionModel::ST_AnchorNode) {
            d->pasteAnchorNodeSelection(playheadPosition);
            return;
        }
        if (!pasteAvailable())
            return;
        const auto type = d->currentClipboardType();
        auto *clipboard = DspxClipboard::instance();
        if (!clipboard || !type.has_value())
            return;

        bool ok = false;
        const auto data = clipboard->paste(type.value(), &ok);
        if (!ok) {
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), nullptr, tr("Paste failed"), tr("Cannot paste data from the clipboard."));
            return;
        }

        qCInfo(lcDspxDocument) << "Pasting" << clipboardTypeName(data.type()) << "playhead" << playheadPosition;

        const QString transactionName = [this, &data] {
            switch (data.type()) {
                case DspxClipboardData::Tempo:
                    return tr("Pasting tempo");
                case DspxClipboardData::Label:
                    return tr("Pasting label");
                case DspxClipboardData::KeySignature:
                    return tr("Pasting key signature");
                case DspxClipboardData::Track:
                    return tr("Pasting track");
                case DspxClipboardData::Clip:
                    return tr("Pasting clip");
                case DspxClipboardData::Note:
                    return tr("Pasting note");
                case DspxClipboardData::DynamicMixingAnchor:
                    return tr("Pasting voice blending anchor");
            }
            return tr("Pasting selection");
        }();

        QList<QObject *> pastedItems;
        bool pasted = false;
        transactionController()->beginScopedTransaction(transactionName, [=, &pasted, &data, &pastedItems] {
            pasted = d->pasteClipboardData(data, playheadPosition, pastedItems);
            return pasted;
        }, [] {
            qCCritical(lcDspxDocument) << "Failed to paste in scoped transaction";
        });

        if (pasted && d->selectionModel) {
            bool first = true;
            for (auto *item : std::as_const(pastedItems)) {
                if (!item)
                    continue;
                const auto command = first
                    ? dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection
                    : dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem;
                d->selectionModel->select(item, command, dspx::SelectionModel::selectionTypeFromItem(item));
                first = false;
            }
        }

        qCInfo(lcDspxDocument) << "Paste result" << clipboardTypeName(data.type()) << (pasted ? "succeeded" : "no changes");
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
                        switch (clip->type()) {
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
        const auto trackBounceInfoMap = buildTrackBounceInfoMap(d->selectionModel);
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
                clearOriginalParameters(info);
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
