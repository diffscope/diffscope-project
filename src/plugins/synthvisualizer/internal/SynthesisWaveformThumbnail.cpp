#include "SynthesisWaveformThumbnail.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include <QLoggingCategory>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <QVector>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/WaveformMipmap.h>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/GlobalAudioContext.h>

namespace SynthVisualizer::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcSynthesisWaveformThumbnail, "diffscope.synthvisualizer.waveformthumbnail")

    SynthesisWaveformThumbnail::SynthesisWaveformThumbnail(QQuickItem *parent)
        : WaveformThumbnail(parent) {
    }

    SynthesisWaveformThumbnail::~SynthesisWaveformThumbnail() {
        ++m_loadRevision;
        for (const auto &connection : std::as_const(m_projectWindowInterfaceConnections)) {
            disconnect(connection);
        }
    }

    QString SynthesisWaveformThumbnail::sourceFilePath() const {
        return m_audioFilePath;
    }

    void SynthesisWaveformThumbnail::setSourceFilePath(const QString &sourceFilePath) {
        if (m_audioFilePath == sourceFilePath) {
            return;
        }
        m_audioFilePath = sourceFilePath;
        loadAudioFile();
        Q_EMIT sourceFilePathChanged();
    }

    Core::ProjectWindowInterface *SynthesisWaveformThumbnail::projectWindowInterface() const {
        return m_projectWindowInterface;
    }

    void SynthesisWaveformThumbnail::setProjectWindowInterface(Core::ProjectWindowInterface *projectWindowInterface) {
        if (m_projectWindowInterface == projectWindowInterface) {
            return;
        }
        for (const auto &connection : std::as_const(m_projectWindowInterfaceConnections)) {
            disconnect(connection);
        }
        m_projectWindowInterfaceConnections.clear();
        m_projectWindowInterface = projectWindowInterface;
        reconnectProjectWindowInterface();
        updateWaveformGeometry();
        Q_EMIT projectWindowInterfaceChanged();
    }

    double SynthesisWaveformThumbnail::startTick() const {
        return m_startTick;
    }

    void SynthesisWaveformThumbnail::setStartTick(double startTick) {
        if (qFuzzyCompare(m_startTick, startTick)) {
            return;
        }
        m_startTick = startTick;
        updateWaveformGeometry();
        Q_EMIT startTickChanged();
    }

    double SynthesisWaveformThumbnail::durationTicks() const {
        return m_durationTicks;
    }

    void SynthesisWaveformThumbnail::setDurationTicks(double durationTicks) {
        if (qFuzzyCompare(m_durationTicks, durationTicks)) {
            return;
        }
        m_durationTicks = durationTicks;
        updateWaveformGeometry();
        Q_EMIT durationTicksChanged();
    }

    double SynthesisWaveformThumbnail::verticalScaleFactor() const {
        return m_verticalScaleFactor;
    }

    void SynthesisWaveformThumbnail::loadAudioFile() {
        const auto revision = ++m_loadRevision;
        clearWaveform();
        if (m_audioFilePath.isEmpty()) {
            return;
        }

        const auto filePath = m_audioFilePath;
        auto formatManager = Audio::GlobalAudioContext::formatManager();
        auto io = std::unique_ptr<talcs::AbstractAudioFormatIO>(
            formatManager ? formatManager->getFormatLoad(filePath) : nullptr
        );
        if (!io || !io->open(talcs::AbstractAudioFormatIO::Read)) {
            qCWarning(lcSynthesisWaveformThumbnail) << "Could not open synthesized audio waveform source";
            return;
        }

        const auto sampleRate = io->sampleRate();
        const auto length = io->length();
        const auto channelCount = io->channelCount();
        const auto maximumVectorSize = std::numeric_limits<qsizetype>::max();
        if (sampleRate <= 0.0 || length < 0 || channelCount <= 0 ||
            length > maximumVectorSize / channelCount) {
            qCWarning(lcSynthesisWaveformThumbnail) << "Synthesized audio has invalid metadata"
                                                    << sampleRate << length << channelCount;
            io->close();
            return;
        }

        QVector<float> interleavedData(static_cast<qsizetype>(length * channelCount));
        const auto readLength = length == 0 ? 0 : io->read(interleavedData.data(), length);
        io->close();
        if (readLength < 0 || readLength > length) {
            qCWarning(lcSynthesisWaveformThumbnail) << "Could not read synthesized audio waveform source";
            return;
        }

        QVector<float> monoData(static_cast<qsizetype>(readLength));
        if (channelCount == 1) {
            std::copy_n(interleavedData.cbegin(), monoData.size(), monoData.begin());
        } else {
            for (qint64 frame = 0; frame < readLength; ++frame) {
                float sum{};
                for (int channel = 0; channel < channelCount; ++channel) {
                    sum += interleavedData.at(static_cast<qsizetype>(frame * channelCount + channel));
                }
                monoData[static_cast<qsizetype>(frame)] = sum / channelCount;
            }
        }

        double maximumAmplitude{};
        for (const auto sample : std::as_const(monoData)) {
            const auto amplitude = std::abs(static_cast<double>(sample));
            if (std::isfinite(amplitude)) {
                maximumAmplitude = std::max(maximumAmplitude, amplitude);
            }
        }
        setVerticalScaleFactor(
            qFuzzyIsNull(maximumAmplitude) ? 1.0 : 1.0 / maximumAmplitude
        );

        QPointer<SynthesisWaveformThumbnail> that(this);
        QThreadPool::globalInstance()->start(QRunnable::create([
            that, revision, filePath, sampleRate, audioData = std::move(monoData)
        ]() mutable {
            SVS::WaveformMipmap mipmap(
                audioData.size(), SVS::WaveformMipmap::Downscale,
                SVS::WaveformMipmap::Int16, false
            );
            mipmap.load(audioData.constData(), 0, audioData.size(), 0);
            if (!that) {
                return;
            }
            QMetaObject::invokeMethod(that, [that, revision, filePath, sampleRate, mipmap] {
                if (!that || that->m_loadRevision != revision ||
                    that->m_audioFilePath != filePath) {
                    return;
                }
                that->m_sampleRate = sampleRate;
                that->setWaveformMipmap(mipmap);
                that->updateWaveformGeometry();
            }, Qt::QueuedConnection);
        }));
    }

    void SynthesisWaveformThumbnail::clearWaveform() {
        m_sampleRate = 0.0;
        setVerticalScaleFactor(1.0);
        setWaveformMipmap({});
        setWaveformOffset(0.0);
        setWaveformSections({});
    }

    void SynthesisWaveformThumbnail::setVerticalScaleFactor(double verticalScaleFactor) {
        if (qFuzzyCompare(m_verticalScaleFactor, verticalScaleFactor)) {
            return;
        }
        m_verticalScaleFactor = verticalScaleFactor;
        Q_EMIT verticalScaleFactorChanged();
    }

    void SynthesisWaveformThumbnail::reconnectProjectWindowInterface() {
        if (!m_projectWindowInterface) {
            return;
        }
        auto projectTimeline = m_projectWindowInterface->projectTimeline();
        auto musicTimeline = projectTimeline ? projectTimeline->musicTimeline() : nullptr;
        if (musicTimeline) {
            m_projectWindowInterfaceConnections.append(connect(
                musicTimeline, &SVS::MusicTimeline::tempiChanged,
                this, &SynthesisWaveformThumbnail::updateWaveformGeometry
            ));
        }
        m_projectWindowInterfaceConnections.append(connect(
            m_projectWindowInterface, &QObject::destroyed, this, [this] {
                m_projectWindowInterface = nullptr;
                m_projectWindowInterfaceConnections.clear();
                setWaveformSections({});
                Q_EMIT projectWindowInterfaceChanged();
            }
        ));
    }

    void SynthesisWaveformThumbnail::updateWaveformGeometry() {
        if (!m_projectWindowInterface || m_sampleRate <= 0.0 || m_durationTicks <= 0.0) {
            setWaveformOffset(0.0);
            setWaveformSections({});
            return;
        }
        auto projectTimeline = m_projectWindowInterface->projectTimeline();
        auto musicTimeline = projectTimeline ? projectTimeline->musicTimeline() : nullptr;
        if (!musicTimeline) {
            setWaveformSections({});
            return;
        }

        const auto endTick = m_startTick + m_durationTicks;
        QList<double> sectionBoundaries{m_startTick};
        auto projectDocumentContext = m_projectWindowInterface->projectDocumentContext();
        auto document = projectDocumentContext ? projectDocumentContext->document() : nullptr;
        auto model = document ? document->model() : nullptr;
        auto tempoSequence = model ? model->tempos() : nullptr;
        if (tempoSequence) {
            const auto sliceStart = std::max(0, static_cast<int>(std::floor(m_startTick)));
            const auto sliceLength = std::max(
                0, static_cast<int>(std::ceil(endTick)) - sliceStart + 1
            );
            for (auto tempo : tempoSequence->slice(sliceStart, sliceLength)) {
                if (tempo && tempo->position() > m_startTick && tempo->position() < endTick) {
                    sectionBoundaries.append(tempo->position());
                }
            }
        }
        sectionBoundaries.append(endTick);
        std::sort(sectionBoundaries.begin(), sectionBoundaries.end());

        QList<SVS::WaveformThumbnailSection> sections;
        sections.reserve(sectionBoundaries.size() - 1);
        for (int index = 0; index + 1 < sectionBoundaries.size(); ++index) {
            const auto start = sectionBoundaries.at(index);
            const auto end = sectionBoundaries.at(index + 1);
            if (end <= start) {
                continue;
            }
            sections.append({
                (start - m_startTick) / m_durationTicks,
                (end - m_startTick) / m_durationTicks,
                tickRangeToSamples(start, end),
            });
        }
        setWaveformOffset(0.0);
        setWaveformSections(sections);
    }

    double SynthesisWaveformThumbnail::tickToMillisecond(double tick) const {
        if (!m_projectWindowInterface) {
            return 0.0;
        }
        auto projectTimeline = m_projectWindowInterface->projectTimeline();
        auto timeline = projectTimeline ? projectTimeline->musicTimeline() : nullptr;
        if (!timeline) {
            return 0.0;
        }
        const auto boundedTick = std::max(0.0, tick);
        const auto wholeTick = static_cast<int>(std::floor(boundedTick));
        auto millisecond = timeline->create(0, 0, wholeTick).millisecond();
        const auto fraction = boundedTick - wholeTick;
        if (!qFuzzyIsNull(fraction)) {
            millisecond += fraction * 60.0 * 1000.0 /
                           (timeline->ticksPerQuarterNote() * timeline->tempoAt(wholeTick));
        }
        return millisecond;
    }

    double SynthesisWaveformThumbnail::tickRangeToSamples(double startTick, double endTick) const {
        if (endTick <= startTick || m_sampleRate <= 0.0) {
            return 0.0;
        }
        return (tickToMillisecond(endTick) - tickToMillisecond(startTick)) *
               m_sampleRate / 1000.0;
    }

}

#include "moc_SynthesisWaveformThumbnail.cpp"
