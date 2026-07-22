#include "WaveformSingerAddOn.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <QJsonObject>
#include <QMap>
#include <QPointF>
#include <QVariant>

#include <SVSCraftCore/MusicTimeline.h>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelORM/VibratoPointDataArray.h>

#include <opendspx/controlpoint.h>
#include <opendspx/vibrato.h>

#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/TrackAudioContext.h>
#include <audio/internal/WaveformSingerAudioSource.h>
#include <audio/internal/WaveformSingerMetadata.h>
#include <audio/internal/WaveformSingerTypeCatalog.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Audio::Internal {

    namespace {

        const QString pitchKey = QStringLiteral("pitch");
        const QString energyKey = QStringLiteral("energy");
        const QString toneShiftKey = QStringLiteral("tone_shift");
        const QString waveformSingerId = QStringLiteral("waveform");

        enum class FreeLayer {
            Original,
            Edited,
            Transform,
        };

        std::vector<double> logicalWeights(const QList<double> &stored, int count) {
            if (count <= 0) {
                return {};
            }
            std::vector<double> result(static_cast<std::size_t>(count), 0.0);
            double sum = 0.0;
            for (int i = 0; i < count - 1; ++i) {
                const auto value = i < stored.size() ? std::clamp(stored.at(i), 0.0, 1.0) : 0.0;
                result[static_cast<std::size_t>(i)] = value;
                sum += value;
            }
            result.back() = std::max(0.0, 1.0 - sum);
            return result;
        }

        WaveformSingerFreeCurve buildFreeCurve(const dspx::FreeValueDataArray *array) {
            WaveformSingerFreeCurve result;
            if (!array) {
                return result;
            }
            const auto values = array->items();
            result.size = values.size();
            const auto blockCount = (values.size() + WaveformSingerFreeCurve::blockSize - 1) /
                                    WaveformSingerFreeCurve::blockSize;
            result.blocks.reserve(static_cast<std::size_t>(blockCount));
            for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
                auto block = std::make_shared<WaveformSingerFreeCurve::Block>();
                const auto begin = blockIndex * WaveformSingerFreeCurve::blockSize;
                const auto end = qMin(values.size(), begin + WaveformSingerFreeCurve::blockSize);
                for (int index = begin; index < end; ++index) {
                    const auto &value = values.at(index);
                    if (!value.isValid() || value.isNull()) {
                        continue;
                    }
                    const auto localIndex = index - begin;
                    block->values[static_cast<std::size_t>(localIndex)] = value.toInt();
                    block->valid[static_cast<std::size_t>(localIndex / 64)] |=
                        std::uint64_t{1} << (localIndex % 64);
                }
                result.blocks.push_back(std::move(block));
            }
            return result;
        }

        WaveformSingerAnchorCurve buildAnchorCurve(const dspx::AnchorNodeSequence *sequence) {
            WaveformSingerAnchorCurve result;
            if (!sequence) {
                return result;
            }
            std::vector<opendspx::AnchorNode> current;
            const auto appendSegment = [&] {
                if (current.empty()) {
                    return;
                }
                result.segments.push_back({
                    current.front().x,
                    current.back().x,
                    std::make_shared<opendspx::ParameterInterpolator>(current),
                });
                current.clear();
            };
            for (const auto node : sequence->asRange()) {
                current.push_back({
                    static_cast<opendspx::AnchorNode::Interpolation>(node->interpolationMode()),
                    node->x(),
                    node->y(),
                });
                if (node->interpolationMode() == dspx::AnchorNode::None) {
                    appendSegment();
                }
            }
            appendSegment();
            return result;
        }

        std::shared_ptr<const WaveformSingerParameterSnapshot> buildParameter(const dspx::Parameter *parameter) {
            auto result = std::make_shared<WaveformSingerParameterSnapshot>();
            if (!parameter) {
                return result;
            }
            result->original = buildFreeCurve(parameter->original());
            result->freeEdited = buildFreeCurve(parameter->freeEdited());
            result->anchorEdited = buildAnchorCurve(parameter->anchorEdited());
            result->freeTransform = buildFreeCurve(parameter->freeTransform());
            result->anchorTransform = buildAnchorCurve(parameter->anchorTransform());
            return result;
        }

        QString singerType(const dspx::Singer *singer) {
            if (!singer->extra().isObject()) {
                return WaveformSingerTypeCatalog::fallbackType();
            }
            const auto object = singer->extra().toObject();
            const auto value = object.value(QStringLiteral("type"));
            return WaveformSingerTypeCatalog::normalizedType(value.isString() ? value.toString() : QString());
        }

        void flattenSinger(const dspx::Singer *singer, double weight,
                           std::array<double, waveformSingerTypeCount> &result,
                           bool &valid, int &leafCount) {
            if (singer->type() == dspx::Singer::Single) {
                const auto single = static_cast<const dspx::SingleSinger *>(singer);
                if (single->id() != waveformSingerId) {
                    valid = false;
                    return;
                }
                ++leafCount;
                const auto index = WaveformSingerTypeCatalog::indexOf(singerType(singer));
                result[static_cast<std::size_t>(index)] += weight;
                return;
            }
            const auto mixed = static_cast<const dspx::MixedSinger *>(singer);
            const auto children = mixed->singers()->items();
            const auto weights = logicalWeights(mixed->ratio(), children.size());
            for (int index = 0; index < children.size(); ++index) {
                flattenSinger(children.at(index), weight * weights[static_cast<std::size_t>(index)],
                              result, valid, leafCount);
            }
        }

        void watchAnchorNodes(dspx::AnchorNodeSequence *sequence, QObject *context,
                              const std::function<void()> &changed,
                              const std::function<void()> &structureChanged) {
            QObject::connect(sequence, &dspx::AnchorNodeSequence::itemInserted, context, structureChanged);
            QObject::connect(sequence, &dspx::AnchorNodeSequence::itemRemoved, context, structureChanged);
            for (const auto node : sequence->asRange()) {
                QObject::connect(node, &dspx::AnchorNode::xChanged, context, changed);
                QObject::connect(node, &dspx::AnchorNode::yChanged, context, changed);
                QObject::connect(node, &dspx::AnchorNode::interpolationModeChanged, context, changed);
            }
        }

        std::vector<opendspx::ControlPoint> controlPoints(const dspx::VibratoPointDataArray *array) {
            const auto source = array->items();
            std::vector<opendspx::ControlPoint> result;
            result.reserve(static_cast<std::size_t>(source.size()));
            for (const auto &point : source) {
                result.push_back({point.x(), point.y()});
            }
            return result;
        }

    }

    class WaveformSingerAddOn::TrackBinding final : public QObject {
    public:
        TrackBinding(WaveformSingerAddOn *addOn, dspx::Track *track)
            : QObject(addOn), m_addOn(addOn), m_track(track),
              m_series(std::make_unique<WaveformSingerTrackAudioSource>(addOn->m_tempoModel)) {
            const auto context = TrackAudioContext::of(track);
            Q_ASSERT(context);
            context->trackMixer()->addSource(m_series.get());
            for (const auto clip : track->clips()->asRange()) {
                m_addOn->addClip(clip, this);
            }
            connect(track->clips(), &dspx::ClipSequence::itemInserted, this,
                    [this](dspx::Clip *clip) { m_addOn->addClip(clip, this); });
            connect(track->clips(), &dspx::ClipSequence::itemRemoved, this,
                    [this](dspx::Clip *clip) { m_addOn->removeClip(clip); });
        }

        ~TrackBinding() override {
            if (const auto context = TrackAudioContext::of(m_track)) {
                context->trackMixer()->removeSource(m_series.get());
            }
        }

        WaveformSingerTrackAudioSource *series() const {
            return m_series.get();
        }

        dspx::Track *track() const {
            return m_track;
        }

    private:
        WaveformSingerAddOn *m_addOn;
        dspx::Track *m_track;
        std::unique_ptr<WaveformSingerTrackAudioSource> m_series;
    };

    class WaveformSingerAddOn::ClipBinding final : public QObject {
    public:
        class NoteBinding final : public QObject {
        public:
            NoteBinding(ClipBinding *clipBinding, dspx::Note *note)
                : QObject(clipBinding), m_clipBinding(clipBinding), m_note(note),
                  m_model(std::make_shared<WaveformSingerNoteModel>()) {
                const auto changed = [this] { rebuild(); };
                connect(note, &dspx::Note::positionChanged, this, changed);
                connect(note, &dspx::Note::lengthChanged, this, changed);
                connect(note, &dspx::Note::keyNumberChanged, this, changed);
                connect(note, &dspx::Note::centShiftChanged, this, changed);
                connect(note, &dspx::Note::vibratoAmplitudeChanged, this, changed);
                connect(note, &dspx::Note::vibratoEndChanged, this, changed);
                connect(note, &dspx::Note::vibratoFrequencyChanged, this, changed);
                connect(note, &dspx::Note::vibratoOffsetChanged, this, changed);
                connect(note, &dspx::Note::vibratoPhaseChanged, this, changed);
                connect(note, &dspx::Note::vibratoStartChanged, this, changed);
                connect(note->vibratoAmplitudeControlPoints(), &dspx::VibratoPointDataArray::itemsChanged,
                        this, changed);
                connect(note->vibratoFrequencyControlPoints(), &dspx::VibratoPointDataArray::itemsChanged,
                        this, changed);
                rebuild();
            }

            ~NoteBinding() override = default;

            void rebuild(bool refreshRange = true) {
                const auto clip = m_clipBinding->m_model->snapshot();
                const auto tempo = m_clipBinding->m_addOn->m_tempoModel->snapshot();
                if (!clip || !tempo) {
                    return;
                }
                const auto startSeconds = tempo->tickToSeconds(clip->startTick + m_note->position());
                const auto endSeconds = tempo->tickToSeconds(clip->startTick + m_note->position() + m_note->length());
                opendspx::Vibrato vibrato{
                    .start = m_note->vibratoStart(),
                    .end = m_note->vibratoEnd(),
                    .amp = m_note->vibratoAmplitude(),
                    .freq = m_note->vibratoFrequency(),
                    .phase = m_note->vibratoPhase(),
                    .offset = m_note->vibratoOffset(),
                    .points = {
                        .amp = controlPoints(m_note->vibratoAmplitudeControlPoints()),
                        .freq = controlPoints(m_note->vibratoFrequencyControlPoints()),
                    },
                };
                auto snapshot = std::make_shared<WaveformSingerNoteSnapshot>();
                snapshot->revision = ++m_clipBinding->m_addOn->m_revision;
                snapshot->positionTick = m_note->position();
                snapshot->lengthTick = m_note->length();
                snapshot->keyNumber = m_note->keyNumber();
                snapshot->centShift = m_note->centShift();
                snapshot->seed = static_cast<std::uint64_t>(m_note->handle().d);
                snapshot->vibrato = std::make_shared<opendspx::VibratoCurve>(
                    std::move(vibrato), std::max(0.0, endSeconds - startSeconds));
                m_model->publish(std::move(snapshot));
                if (refreshRange && m_source && m_clipBinding->m_backend) {
                    m_clipBinding->m_backend->refreshNoteRange(
                        m_source, m_clipBinding->m_addOn->activeSampleRate());
                }
            }

            std::shared_ptr<WaveformSingerNoteModel> model() const {
                return m_model;
            }

            void setSource(WaveformSingerNoteAudioSource *source) {
                m_source = source;
            }

            WaveformSingerNoteAudioSource *source() const {
                return m_source;
            }

        private:
            ClipBinding *m_clipBinding;
            dspx::Note *m_note;
            std::shared_ptr<WaveformSingerNoteModel> m_model;
            WaveformSingerNoteAudioSource *m_source{};
        };

        ClipBinding(WaveformSingerAddOn *addOn, TrackBinding *trackBinding, dspx::SingingClip *clip)
            : QObject(addOn), m_addOn(addOn), m_trackBinding(trackBinding), m_clip(clip),
              m_model(std::make_shared<WaveformSingerClipModel>()) {
            const auto geometryChanged = [this] { syncGeometry(); };
            connect(clip, &dspx::Clip::positionChanged, this, geometryChanged);
            connect(clip, &dspx::Clip::lengthChanged, this, geometryChanged);
            connect(clip, &dspx::Clip::clipStartChanged, this, geometryChanged);
            connect(clip, &dspx::Clip::clipLengthChanged, this, geometryChanged);
            connect(clip, &dspx::Clip::gainChanged, this, [this] { syncControl(); });
            connect(clip, &dspx::Clip::panChanged, this, [this] { syncControl(); });
            connect(clip, &dspx::Clip::muteChanged, this, [this] { syncControl(); });
            connect(clip, &dspx::SingingClip::sourcesChanged, this, [this] { syncSources(); });

            syncGeometry();
            syncParameters();
            for (const auto note : clip->notes()->asRange()) {
                addNote(note);
            }
            connect(clip->notes(), &dspx::NoteSequence::itemInserted, this,
                    [this](dspx::Note *note) { addNote(note); });
            connect(clip->notes(), &dspx::NoteSequence::itemRemoved, this,
                    [this](dspx::Note *note) { removeNote(note); });
            syncSources();
        }

        ~ClipBinding() override {
            setEligible(false);
        }

        void tempoChanged() {
            syncGeometry();
            for (const auto binding : std::as_const(m_notes)) {
                binding->rebuild(false);
            }
            refreshRanges();
        }

        void refreshRanges() {
            if (m_backend) {
                m_backend->refreshRanges(m_trackBinding->series(), m_addOn->activeSampleRate());
            }
        }

    private:
        void publishClip(std::shared_ptr<WaveformSingerClipSnapshot> snapshot, bool phaseChanged) {
            snapshot->revision = ++m_addOn->m_revision;
            if (phaseChanged) {
                snapshot->phaseRevision = snapshot->revision;
            }
            m_model->publish(std::move(snapshot));
        }

        std::shared_ptr<WaveformSingerClipSnapshot> mutableSnapshot() const {
            const auto current = m_model->snapshot();
            return current ? std::make_shared<WaveformSingerClipSnapshot>(*current)
                           : std::make_shared<WaveformSingerClipSnapshot>();
        }

        void syncGeometry() {
            auto snapshot = mutableSnapshot();
            snapshot->startTick = m_clip->start();
            snapshot->positionTick = m_clip->position();
            snapshot->contentLengthTick = m_clip->length();
            snapshot->clipStartTick = m_clip->clipStart();
            snapshot->clipLengthTick = m_clip->clipLength();
            publishClip(std::move(snapshot), true);
            refreshRanges();
        }

        void syncControl() {
            if (!m_backend) {
                return;
            }
            m_backend->controlMixer()->setGain(static_cast<float>(m_clip->gain()));
            m_backend->controlMixer()->setPan(static_cast<float>(m_clip->pan()));
            m_backend->controlMixer()->setSilentFlags(m_clip->mute() ? -1 : 0);
        }

        void syncParameters() {
            if (m_parameterWatcher) {
                m_parameterWatcher->deleteLater();
            }
            m_parameterWatcher = new QObject(this);
            const auto map = m_clip->parameters();
            connect(map, &dspx::ParameterMap::itemInserted, m_parameterWatcher, [this] { syncParameters(); });
            connect(map, &dspx::ParameterMap::itemRemoved, m_parameterWatcher, [this] { syncParameters(); });
            watchParameter(pitchKey, map->item(pitchKey));
            watchParameter(energyKey, map->item(energyKey));
            watchParameter(toneShiftKey, map->item(toneShiftKey));

            auto snapshot = mutableSnapshot();
            snapshot->pitch = buildParameter(map->item(pitchKey));
            snapshot->energy = buildParameter(map->item(energyKey));
            snapshot->toneShift = buildParameter(map->item(toneShiftKey));
            publishClip(std::move(snapshot), true);
        }

        void watchParameter(const QString &key, dspx::Parameter *parameter) {
            if (!parameter) {
                return;
            }
            const auto changed = [this, key] { rebuildParameter(key); };
            const auto structureChanged = [this] { syncParameters(); };
            watchFreeCurve(key, FreeLayer::Original, parameter->original());
            watchFreeCurve(key, FreeLayer::Edited, parameter->freeEdited());
            watchFreeCurve(key, FreeLayer::Transform, parameter->freeTransform());
            watchAnchorNodes(parameter->anchorEdited(), m_parameterWatcher, changed, structureChanged);
            watchAnchorNodes(parameter->anchorTransform(), m_parameterWatcher, changed, structureChanged);
        }

        void watchFreeCurve(const QString &key, FreeLayer layer, dspx::FreeValueDataArray *array) {
            connect(array, &dspx::FreeValueDataArray::spliced, m_parameterWatcher,
                    [this, key, layer, array](int index, int length, const QList<QVariant> &values) {
                        patchFreeCurve(key, layer, array, index, length, values);
                    });
            connect(array, &dspx::FreeValueDataArray::rotated, m_parameterWatcher,
                    [this, key] { rebuildParameter(key); });
        }

        void patchFreeCurve(const QString &key, FreeLayer layer, dspx::FreeValueDataArray *array,
                            int index, int length, const QList<QVariant> &values) {
            const auto clipSnapshot = m_model->snapshot();
            const auto oldParameter = key == pitchKey ? clipSnapshot->pitch
                                    : key == energyKey ? clipSnapshot->energy
                                                       : clipSnapshot->toneShift;
            auto parameter = oldParameter
                ? std::make_shared<WaveformSingerParameterSnapshot>(*oldParameter)
                : std::make_shared<WaveformSingerParameterSnapshot>();
            auto *curve = layer == FreeLayer::Original ? &parameter->original
                        : layer == FreeLayer::Edited ? &parameter->freeEdited
                                                     : &parameter->freeTransform;
            if (length != values.size() || curve->size != array->size() || index < 0) {
                *curve = buildFreeCurve(array);
            } else if (!values.isEmpty()) {
                const auto firstBlock = index / WaveformSingerFreeCurve::blockSize;
                const auto lastBlock = (index + values.size() - 1) / WaveformSingerFreeCurve::blockSize;
                for (int blockIndex = firstBlock; blockIndex <= lastBlock; ++blockIndex) {
                    curve->blocks[static_cast<std::size_t>(blockIndex)] =
                        std::make_shared<WaveformSingerFreeCurve::Block>(
                            *curve->blocks[static_cast<std::size_t>(blockIndex)]);
                }
                for (int offset = 0; offset < values.size(); ++offset) {
                    const auto targetIndex = index + offset;
                    const auto blockIndex = static_cast<std::size_t>(
                        targetIndex / WaveformSingerFreeCurve::blockSize);
                    const auto localIndex = targetIndex % WaveformSingerFreeCurve::blockSize;
                    auto block = std::const_pointer_cast<WaveformSingerFreeCurve::Block>(
                        curve->blocks[blockIndex]);
                    const auto mask = std::uint64_t{1} << (localIndex % 64);
                    auto &word = block->valid[static_cast<std::size_t>(localIndex / 64)];
                    const auto &value = values.at(offset);
                    if (!value.isValid() || value.isNull()) {
                        word &= ~mask;
                    } else {
                        block->values[static_cast<std::size_t>(localIndex)] = value.toInt();
                        word |= mask;
                    }
                }
            }
            auto snapshot = mutableSnapshot();
            if (key == pitchKey) {
                snapshot->pitch = std::move(parameter);
            } else if (key == energyKey) {
                snapshot->energy = std::move(parameter);
            } else {
                snapshot->toneShift = std::move(parameter);
            }
            publishClip(std::move(snapshot), key != energyKey);
        }

        void rebuildParameter(const QString &key) {
            auto snapshot = mutableSnapshot();
            const auto parameter = buildParameter(m_clip->parameters()->item(key));
            if (key == pitchKey) {
                snapshot->pitch = parameter;
            } else if (key == energyKey) {
                snapshot->energy = parameter;
            } else {
                snapshot->toneShift = parameter;
            }
            publishClip(std::move(snapshot), key != energyKey);
        }

        void watchSingerList(dspx::SingerList *list) {
            connect(list, &dspx::SingerList::itemInserted, m_sourceWatcher, [this] { syncSources(); });
            connect(list, &dspx::SingerList::itemRemoved, m_sourceWatcher, [this] { syncSources(); });
            connect(list, &dspx::SingerList::rotated, m_sourceWatcher, [this] { syncSources(); });
            for (const auto singer : list->items()) {
                connect(singer, &dspx::Singer::extraChanged, m_sourceWatcher, [this] { syncSources(); });
                if (singer->type() == dspx::Singer::Single) {
                    connect(static_cast<dspx::SingleSinger *>(singer), &dspx::SingleSinger::idChanged,
                            m_sourceWatcher, [this] { syncSources(); });
                } else {
                    const auto mixed = static_cast<dspx::MixedSinger *>(singer);
                    connect(mixed, &dspx::MixedSinger::ratioChanged, m_sourceWatcher, [this] { syncSources(); });
                    watchSingerList(mixed->singers());
                }
            }
        }

        void syncSources() {
            if (m_sourceWatcher) {
                m_sourceWatcher->deleteLater();
            }
            m_sourceWatcher = new QObject(this);
            auto voices = std::make_shared<WaveformSingerVoiceSnapshot>();
            const auto sources = m_clip->sources();
            bool valid = sources && sources->category() == WaveformSingerMetadata::architectureId();
            int leafCount = 0;
            if (sources) {
                connect(sources, &dspx::Sources::categoryChanged, m_sourceWatcher, [this] { syncSources(); });
                watchSingerList(sources->singers());
                const auto rootSingers = sources->singers()->items();
                voices->roots.reserve(static_cast<std::size_t>(rootSingers.size()));
                for (const auto singer : rootSingers) {
                    std::array<double, waveformSingerTypeCount> root{};
                    flattenSinger(singer, 1.0, root, valid, leafCount);
                    voices->roots.push_back(root);
                }
                const auto dynamicAnchors = sources->dynamicMixingAnchors();
                connect(dynamicAnchors, &dspx::DynamicMixingAnchorSequence::itemInserted,
                        m_sourceWatcher, [this] { syncSources(); });
                connect(dynamicAnchors, &dspx::DynamicMixingAnchorSequence::itemRemoved,
                        m_sourceWatcher, [this] { syncSources(); });
                for (const auto anchor : dynamicAnchors->asRange()) {
                    connect(anchor, &dspx::DynamicMixingAnchor::positionChanged,
                            m_sourceWatcher, [this] { syncSources(); });
                    connect(anchor, &dspx::DynamicMixingAnchor::ratioChanged,
                            m_sourceWatcher, [this] { syncSources(); });
                    voices->anchors.push_back({
                        anchor->position(),
                        logicalWeights(anchor->ratio(), rootSingers.size()),
                    });
                }
            }
            valid = valid && !voices->roots.empty() && leafCount > 0;
            auto snapshot = mutableSnapshot();
            snapshot->voices = std::move(voices);
            publishClip(std::move(snapshot), false);
            setEligible(valid);
        }

        void setEligible(bool eligible) {
            if (eligible == (m_backend != nullptr)) {
                if (eligible) {
                    syncControl();
                    refreshRanges();
                }
                return;
            }
            if (!eligible) {
                for (const auto binding : std::as_const(m_notes)) {
                    binding->setSource(nullptr);
                }
                m_trackBinding->series()->removeClip(m_backend);
                m_backend = nullptr;
                return;
            }
            m_backend = m_trackBinding->series()->addClip(m_model);
            for (const auto binding : std::as_const(m_notes)) {
                binding->setSource(m_backend->addNote(binding->model()));
            }
            syncControl();
            refreshRanges();
        }

        void addNote(dspx::Note *note) {
            if (m_notes.contains(note)) {
                return;
            }
            auto binding = new NoteBinding(this, note);
            m_notes.insert(note, binding);
            if (m_backend) {
                binding->setSource(m_backend->addNote(binding->model()));
                refreshRanges();
            }
        }

        void removeNote(dspx::Note *note) {
            const auto binding = m_notes.take(note);
            if (!binding) {
                return;
            }
            if (m_backend && binding->source()) {
                m_backend->removeNote(binding->source());
            }
            delete binding;
            refreshRanges();
        }

        WaveformSingerAddOn *m_addOn;
        TrackBinding *m_trackBinding;
        dspx::SingingClip *m_clip;
        std::shared_ptr<WaveformSingerClipModel> m_model;
        WaveformSingerClipAudioSource *m_backend{};
        QHash<dspx::Note *, NoteBinding *> m_notes;
        QObject *m_parameterWatcher{};
        QObject *m_sourceWatcher{};
    };

    WaveformSingerAddOn::WaveformSingerAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    WaveformSingerAddOn::~WaveformSingerAddOn() {
        const auto clips = m_clips.keys();
        for (const auto clip : clips) {
            removeClip(clip);
        }
        qDeleteAll(m_tracks);
        m_tracks.clear();
    }

    void WaveformSingerAddOn::initialize() {
        const auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        window->addObject(this);
        m_audioContext = ProjectAudioContext::of(window);
        Q_ASSERT(m_audioContext);
        m_tempoModel = std::make_shared<WaveformSingerTempoModel>();
        syncTempo();

        const auto trackList = window->projectDocumentContext()->document()->model()->tracks();
        const auto tracks = trackList->items();
        for (int index = 0; index < tracks.size(); ++index) {
            addTrack(index, tracks.at(index));
        }
        connect(trackList, &dspx::TrackList::itemInserted, this, &WaveformSingerAddOn::addTrack);
        connect(trackList, &dspx::TrackList::itemRemoved, this, &WaveformSingerAddOn::removeTrack);

        const auto timeline = window->projectTimeline()->musicTimeline();
        connect(timeline, &SVS::MusicTimeline::tempiChanged, this, &WaveformSingerAddOn::syncTempo);
        connect(timeline, &SVS::MusicTimeline::ticksPerQuarterNoteChanged, this, &WaveformSingerAddOn::syncTempo);
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::sampleRateChanged,
                this, &WaveformSingerAddOn::refreshRanges);
    }

    void WaveformSingerAddOn::extensionsInitialized() {
    }

    bool WaveformSingerAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    void WaveformSingerAddOn::addTrack(int index, dspx::Track *track) {
        Q_UNUSED(index)
        if (m_tracks.contains(track)) {
            return;
        }
        m_tracks.insert(track, new TrackBinding(this, track));
    }

    void WaveformSingerAddOn::removeTrack(int index, dspx::Track *track) {
        Q_UNUSED(index)
        const auto binding = m_tracks.take(track);
        if (!binding) {
            return;
        }
        const auto clips = track->clips()->asRange();
        for (const auto clip : clips) {
            removeClip(clip);
        }
        delete binding;
    }

    void WaveformSingerAddOn::addClip(dspx::Clip *clip, TrackBinding *trackBinding) {
        if (!clip || clip->type() != dspx::Clip::Singing) {
            return;
        }
        auto singingClip = static_cast<dspx::SingingClip *>(clip);
        removeClip(clip);
        m_clips.insert(singingClip, new ClipBinding(this, trackBinding, singingClip));
    }

    void WaveformSingerAddOn::removeClip(dspx::Clip *clip) {
        if (!clip || clip->type() != dspx::Clip::Singing) {
            return;
        }
        delete m_clips.take(static_cast<dspx::SingingClip *>(clip));
    }

    void WaveformSingerAddOn::syncTempo() {
        const auto timeline = windowHandle()->cast<Core::ProjectWindowInterface>()
                                  ->projectTimeline()->musicTimeline();
        auto tempi = timeline->tempi();
        if (!tempi.contains(0)) {
            tempi.insert(0, timeline->tempoAt(0));
        }
        auto snapshot = std::make_shared<WaveformSingerTempoSnapshot>();
        snapshot->revision = ++m_revision;
        snapshot->ticksPerQuarter = timeline->ticksPerQuarterNote();
        double previousTick = 0.0;
        double previousSeconds = 0.0;
        double previousTempo = tempi.value(0, 120.0);
        for (auto it = tempi.cbegin(); it != tempi.cend(); ++it) {
            if (it.key() < 0) {
                continue;
            }
            previousSeconds += (it.key() - previousTick) * 60.0 /
                               (previousTempo * snapshot->ticksPerQuarter);
            snapshot->segments.push_back({
                static_cast<double>(it.key()), previousSeconds, it.value(),
            });
            previousTick = it.key();
            previousTempo = it.value();
        }
        if (snapshot->segments.empty()) {
            snapshot->segments.push_back({0.0, 0.0, 120.0});
        }
        m_tempoModel->publish(std::move(snapshot));
        for (const auto clip : std::as_const(m_clips)) {
            clip->tempoChanged();
        }
        refreshRanges();
    }

    void WaveformSingerAddOn::refreshRanges() {
        const auto sampleRate = activeSampleRate();
        if (qFuzzyIsNull(sampleRate)) {
            return;
        }
        for (const auto track : std::as_const(m_tracks)) {
            track->series()->refreshRanges(sampleRate);
        }
    }

    double WaveformSingerAddOn::activeSampleRate() const {
        if (!m_audioContext) {
            return 0.0;
        }
        const auto active = m_audioContext->preMixer()->sampleRate();
        return qFuzzyIsNull(active) ? GlobalAudioContext::sampleRate() : active;
    }

}
