#include "AudioThumbnailWaveformThumbnail.h"

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audiovisualizer/internal/AudioMipmapAddOn.h>

namespace AudioVisualizer::Internal {

    AudioThumbnailWaveformThumbnail::AudioThumbnailWaveformThumbnail(QQuickItem *parent) : WaveformThumbnail(parent) {
    }

    AudioThumbnailWaveformThumbnail::~AudioThumbnailWaveformThumbnail() {
        resetMapping();
    }

    dspx::AudioClip *AudioThumbnailWaveformThumbnail::audioClip() const {
        return m_audioClip;
    }

    void AudioThumbnailWaveformThumbnail::setAudioClip(dspx::AudioClip *audioClip) {
        if (m_audioClip == audioClip) {
            return;
        }
        resetMapping();
        m_audioClip = audioClip;
        reconnectProjectWindowInterface();
        reconnectAudioClip();
        updateMapping();
        updateWaveformGeometry();
        Q_EMIT audioClipChanged();
    }

    Core::ProjectWindowInterface *AudioThumbnailWaveformThumbnail::projectWindowInterface() const {
        return m_projectWindowInterface;
    }

    void AudioThumbnailWaveformThumbnail::setProjectWindowInterface(Core::ProjectWindowInterface *projectWindowInterface) {
        if (m_projectWindowInterface == projectWindowInterface) {
            return;
        }
        resetMapping();
        m_projectWindowInterface = projectWindowInterface;
        reconnectAudioClip();
        reconnectProjectWindowInterface();
        updateMapping();
        updateWaveformGeometry();
        Q_EMIT projectWindowInterfaceChanged();
    }

    double AudioThumbnailWaveformThumbnail::viewportOffset() const {
        return m_viewportOffset;
    }

    void AudioThumbnailWaveformThumbnail::setViewportOffset(double viewportOffset) {
        if (qFuzzyCompare(m_viewportOffset, viewportOffset)) {
            return;
        }
        m_viewportOffset = viewportOffset;
        updateWaveformGeometry();
        Q_EMIT viewportOffsetChanged();
    }

    double AudioThumbnailWaveformThumbnail::viewportLength() const {
        return m_viewportLength;
    }

    void AudioThumbnailWaveformThumbnail::setViewportLength(double viewportLength) {
        if (qFuzzyCompare(m_viewportLength, viewportLength)) {
            return;
        }
        m_viewportLength = viewportLength;
        updateWaveformGeometry();
        Q_EMIT viewportLengthChanged();
    }

    double AudioThumbnailWaveformThumbnail::sampleRate() const {
        return m_sampleRate;
    }

    void AudioThumbnailWaveformThumbnail::setSampleRate(double sampleRate) {
        if (qFuzzyCompare(m_sampleRate, sampleRate)) {
            return;
        }
        m_sampleRate = sampleRate;
        updateWaveformGeometry();
        Q_EMIT sampleRateChanged();
    }

    void AudioThumbnailWaveformThumbnail::resetMapping() {
        if (m_mipmapAddOn && m_audioClip) {
            m_mipmapAddOn->unmapThumbnailVIew(m_audioClip, this);
        }
        m_mipmapAddOn.clear();
        for (const auto &connection : std::as_const(m_audioClipConnections)) {
            disconnect(connection);
        }
        m_audioClipConnections.clear();
        for (const auto &connection : std::as_const(m_projectWindowInterfaceConnections)) {
            disconnect(connection);
        }
        m_projectWindowInterfaceConnections.clear();
    }

    void AudioThumbnailWaveformThumbnail::updateMapping() {
        if (!m_audioClip || !m_projectWindowInterface) {
            clearWaveformGeometry();
            return;
        }
        m_mipmapAddOn = AudioMipmapAddOn::of(m_projectWindowInterface);
        if (m_mipmapAddOn) {
            m_mipmapAddOn->mapThumbnailView(m_audioClip, this);
        }
    }

    void AudioThumbnailWaveformThumbnail::reconnectAudioClip() {
        if (!m_audioClip) {
            return;
        }
        m_audioClipConnections.append(connect(m_audioClip, &dspx::Clip::positionChanged, this, [this] {
            updateWaveformGeometry();
        }));
        m_audioClipConnections.append(connect(m_audioClip, &dspx::Clip::lengthChanged, this, [this] {
            updateWaveformGeometry();
        }));
        m_audioClipConnections.append(connect(m_audioClip, &QObject::destroyed, this, [this] {
            m_audioClip.clear();
            resetMapping();
            clearWaveformGeometry();
            Q_EMIT audioClipChanged();
        }));
    }

    void AudioThumbnailWaveformThumbnail::reconnectProjectWindowInterface() {
        if (!m_projectWindowInterface) {
            return;
        }
        auto timeline = m_projectWindowInterface->projectTimeline() ? m_projectWindowInterface->projectTimeline()->musicTimeline() : nullptr;
        if (timeline) {
            m_projectWindowInterfaceConnections.append(connect(timeline, &SVS::MusicTimeline::tempiChanged, this, [this] {
                updateWaveformGeometry();
            }));
        }
        m_projectWindowInterfaceConnections.append(connect(m_projectWindowInterface, &QObject::destroyed, this, [this] {
            m_projectWindowInterface.clear();
            resetMapping();
            clearWaveformGeometry();
            Q_EMIT projectWindowInterfaceChanged();
        }));
    }

    void AudioThumbnailWaveformThumbnail::updateWaveformGeometry() {
        if (!m_audioClip || !m_projectWindowInterface || m_sampleRate <= 0.0 || m_viewportLength <= 0.0) {
            clearWaveformGeometry();
            return;
        }
        if (!m_projectWindowInterface->projectTimeline() || !m_projectWindowInterface->projectTimeline()->musicTimeline()) {
            clearWaveformGeometry();
            return;
        }

        if (m_audioClip->clipLength() <= 0) {
            clearWaveformGeometry();
            return;
        }

        const double clipPosition = m_audioClip->position();
        const double clipStart = m_audioClip->clipStart();
        const double clipLength = m_audioClip->clipLength();
        const double clipEnd = clipPosition + clipLength;
        const double viewportStart = clipPosition + m_viewportOffset;
        const double viewportEnd = viewportStart + m_viewportLength;
        const double sectionStart = qMax(clipPosition, viewportStart);
        const double sectionEnd = qMin(clipEnd, viewportEnd);
        if (sectionEnd <= sectionStart) {
            clearWaveformGeometry();
            return;
        }

        const double audioZeroPosition = clipPosition - clipStart;
        setWaveformOffset(tickRangeToSamples(audioZeroPosition, sectionStart));

        QList<double> sectionBoundaries{sectionStart};
        auto projectDocumentContext = m_projectWindowInterface->projectDocumentContext();
        auto document = projectDocumentContext ? projectDocumentContext->document() : nullptr;
        auto model = document ? document->model() : nullptr;
        auto tempoSequence = model ? model->tempos() : nullptr;
        if (tempoSequence) {
            const auto sliceStart = qMax(0, static_cast<int>(std::floor(sectionStart)));
            const auto sliceLength = qMax(0, static_cast<int>(std::ceil(sectionEnd)) - sliceStart + 1);
            const auto tempos = tempoSequence->slice(sliceStart, sliceLength);
            for (auto tempo : tempos) {
                if (!tempo) {
                    continue;
                }
                const double tempoPosition = tempo->position();
                if (tempoPosition > sectionStart && tempoPosition < sectionEnd) {
                    sectionBoundaries.append(tempoPosition);
                }
            }
        }
        sectionBoundaries.append(sectionEnd);
        std::sort(sectionBoundaries.begin(), sectionBoundaries.end());

        QList<SVS::WaveformThumbnailSection> sections;
        sections.reserve(sectionBoundaries.size() > 1 ? sectionBoundaries.size() - 1 : 0);
        for (int i = 0; i + 1 < sectionBoundaries.size(); ++i) {
            const auto start = sectionBoundaries.at(i);
            const auto end = sectionBoundaries.at(i + 1);
            if (end <= start) {
                continue;
            }
            sections.append({
                (start - clipPosition) / clipLength,
                (end - clipPosition) / clipLength,
                tickRangeToSamples(start, end),
            });
        }
        setWaveformSections(sections);
    }

    void AudioThumbnailWaveformThumbnail::clearWaveformGeometry() {
        setWaveformOffset(0.0);
        setWaveformSections({});
    }

    double AudioThumbnailWaveformThumbnail::tickToMillisecond(double tick) const {
        if (!m_projectWindowInterface || !m_projectWindowInterface->projectTimeline()) {
            return 0.0;
        }
        auto timeline = m_projectWindowInterface->projectTimeline()->musicTimeline();
        if (!timeline) {
            return 0.0;
        }

        const double boundedTick = qMax(0.0, tick);
        const auto wholeTick = static_cast<int>(std::floor(boundedTick));
        auto millisecond = timeline->create(0, 0, wholeTick).millisecond();
        const auto fraction = boundedTick - wholeTick;
        if (!qFuzzyIsNull(fraction)) {
            millisecond += fraction * 60.0 * 1000.0 / (timeline->ticksPerQuarterNote() * timeline->tempoAt(wholeTick));
        }
        return millisecond;
    }

    double AudioThumbnailWaveformThumbnail::tickRangeToSamples(double startTick, double endTick) const {
        if (endTick <= startTick || m_sampleRate <= 0.0) {
            return 0.0;
        }
        return (tickToMillisecond(endTick) - tickToMillisecond(startTick)) * m_sampleRate / 1000.0;
    }

}

#include "moc_AudioThumbnailWaveformThumbnail.cpp"
