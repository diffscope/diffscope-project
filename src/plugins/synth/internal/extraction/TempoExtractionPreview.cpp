// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TempoExtractionPreview.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

#include <QFutureWatcher>
#include <QPointF>
#include <QtConcurrentRun>

#include <SVSCraftCore/LongTime.h>
#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftGui/LongTimeValidator.h>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsCore/TransportAudioSource.h>
#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <synth/internal/TempoExtractionAudioSource.h>

namespace Synth::Internal {

    TempoExtractionPreview::TempoExtractionPreview(const TempoExtractionInput &input, const ExtractionAudioCodec::Input &audio, Core::ProjectWindowInterface *window, QObject *parent)
        : QObject(parent), m_input(input), m_audio(audio), m_window(window), m_timeline(new SVS::MusicTimeline(input.resolution, this)),
          m_startValidator(new SVS::LongTimeValidator(this)), m_endValidator(new SVS::LongTimeValidator(this)), m_timeValidator(new SVS::LongTimeValidator(this)),
          m_waveformCancelled(std::make_shared<std::atomic_bool>(false)),
          m_viewStart(input.startSeconds), m_viewEnd(input.startSeconds + input.durationSeconds), m_playhead(input.startSeconds) {
        QList<QPair<int, double>> tempoList;
        for (auto it = input.tempi.cbegin(); it != input.tempi.cend(); ++it)
            tempoList.append({it.key(), it.value()});
        m_timeline->setTempi(tempoList);
        QList<QPair<int, SVS::MusicTimeSignature>> signatureList;
        for (auto it = input.timeSignatures.cbegin(); it != input.timeSignatures.cend(); ++it)
            signatureList.append({it.key(), it.value()});
        m_timeline->setTimeSignatures(signatureList);
        for (qsizetype i = 0; i < input.beats.size(); ++i) {
            if (input.beats[i].downbeat) {
                m_options.referenceBeat = static_cast<int>(i);
                break;
            }
        }
        m_startValidator->setRange(SVS::LongTime(qRound(begin() * 1000)), SVS::LongTime(qRound(end() * 1000) - 1));
        m_endValidator->setRange(SVS::LongTime(qRound(begin() * 1000) + 1), SVS::LongTime(qRound(end() * 1000)));
        m_timeValidator->setRange(SVS::LongTime(qRound(begin() * 1000)), SVS::LongTime(qRound(end() * 1000)));
        m_calculationTimer.setSingleShot(true);
        m_calculationTimer.setInterval(150);
        connect(&m_calculationTimer, &QTimer::timeout, this, &TempoExtractionPreview::calculate);
        m_playbackTimer.setInterval(30);
        connect(&m_playbackTimer, &QTimer::timeout, this, [this] {
            if (!m_transport)
                return;
            m_playhead = std::clamp(begin() + m_transport->position() / m_outputRate, begin(), end());
            if (!m_loop && m_transport->position() >= m_source->length()) {
                pause();
                m_playhead = m_viewEnd;
            }
            Q_EMIT playbackChanged();
        });
        connect(Audio::GlobalAudioContext::instance(), &Audio::GlobalAudioContext::sampleRateChanged, this, &TempoExtractionPreview::releasePlayback);
        connect(Audio::GlobalAudioContext::instance(), &Audio::GlobalAudioContext::deviceChanged, this, &TempoExtractionPreview::releasePlayback);
        loadWaveform();
        calculate();
    }

    TempoExtractionPreview::~TempoExtractionPreview() {
        if (m_cancelled)
            m_cancelled->store(true);
        m_waveformCancelled->store(true);
        releasePlayback();
    }

    bool TempoExtractionPreview::busy() const { return m_busy; }
    bool TempoExtractionPreview::valid() const { return !m_busy && m_plan.error.isEmpty() && !m_plan.tempi.isEmpty(); }
    QString TempoExtractionPreview::error() const { return m_plan.error.isEmpty() ? m_audioError : m_plan.error; }

    int TempoExtractionPreview::tempoChangeCount() const { return m_plan.error.isEmpty() && !m_plan.tempi.isEmpty() ? m_plan.fittedCount : 0; }
    double TempoExtractionPreview::maximumOffsetMsec() const { return m_plan.maxErrorMsec; }
    bool TempoExtractionPreview::canKeepAudioPosition() const { return m_plan.keepAudioPositionError.isEmpty(); }
    QString TempoExtractionPreview::alignmentHint() const { return m_plan.keepAudioPositionError; }

    bool TempoExtractionPreview::variable() const { return m_options.variable; }
    void TempoExtractionPreview::setVariable(bool value) {
        if (m_options.variable == value)
            return;
        m_options.variable = value;
        schedule();
    }

    bool TempoExtractionPreview::automatic() const { return m_options.automatic; }
    void TempoExtractionPreview::setAutomatic(bool value) {
        if (m_options.automatic == value)
            return;
        if (!value && m_plan.error.isEmpty() && !m_plan.tempi.isEmpty() && m_plan.bpm > 0)
            m_options.bpm = m_plan.bpm;
        m_options.automatic = value;
        schedule();
    }

    double TempoExtractionPreview::bpm() const { return m_options.automatic && !m_options.variable && m_plan.bpm > 0 ? m_plan.bpm : m_options.bpm; }
    void TempoExtractionPreview::setBpm(double value) {
        if (!std::isfinite(value) || value < 10 || value > 1000 || m_options.bpm == value)
            return;
        m_options.bpm = value;
        schedule();
    }

    void TempoExtractionPreview::scaleBpm(double factor) {
        if (!valid() || variable() || (factor != 0.5 && factor != 2))
            return;
        const auto value = bpm() * factor;
        if (!std::isfinite(value) || value < 10 || value > 1000)
            return;
        m_options.automatic = false;
        m_options.bpm = value;
        schedule();
    }

    double TempoExtractionPreview::smoothing() const { return m_options.smoothing; }
    void TempoExtractionPreview::setSmoothing(double value) {
        if (!std::isfinite(value) || value < 0 || value > 100 || m_options.smoothing == value)
            return;
        m_options.smoothing = value;
        schedule();
    }

    int TempoExtractionPreview::minimumSegmentBeats() const { return m_options.minimumSegmentBeats; }
    void TempoExtractionPreview::setMinimumSegmentBeats(int value) {
        if (value < 1 || value > 64 || m_options.minimumSegmentBeats == value)
            return;
        m_options.minimumSegmentBeats = value;
        schedule();
    }

    bool TempoExtractionPreview::moveAudio() const { return m_options.moveAudio; }
    void TempoExtractionPreview::setMoveAudio(bool value) {
        if (m_options.moveAudio == value || (!value && !canKeepAudioPosition()))
            return;
        m_options.moveAudio = value;
        schedule();
    }

    int TempoExtractionPreview::referenceBeat() const { return m_options.referenceBeat; }
    void TempoExtractionPreview::setReferenceBeat(int value) {
        if (value < 0 || value >= m_input.beats.size() || value == m_options.referenceBeat)
            return;
        if (m_plan.beatTicks.size() == m_input.beats.size()) {
            const auto tick = static_cast<qint64>(m_options.referenceTick) + m_plan.beatTicks[value] - m_plan.beatTicks[m_options.referenceBeat];
            if (tick < 0 || tick >= std::numeric_limits<int>::max())
                return;
            m_options.referenceTick = static_cast<int>(tick);
        }
        m_options.referenceBeat = value;
        schedule();
    }

    int TempoExtractionPreview::referenceTick() const { return m_options.referenceTick; }
    void TempoExtractionPreview::setReferenceTick(int value) {
        if (value < 0 || (value == m_options.referenceTick && !m_options.automaticReference))
            return;
        m_options.automaticReference = false;
        m_options.referenceTick = value;
        schedule();
    }

    QVariantList TempoExtractionPreview::beatLabels() const {
        QVariantList result;
        result.reserve(m_input.beats.size());
        for (qsizetype i = 0; i < m_input.beats.size(); ++i)
            result.append(tr("%L1. %2%3").arg(i + 1).arg(SVS::LongTime(qRound((begin() + m_input.beats[i].seconds) * 1000)).toString(), m_input.beats[i].downbeat ? tr(" (Downbeat)") : QString()));
        return result;
    }

    SVS::MusicTimeline *TempoExtractionPreview::timeline() const { return m_timeline; }
    double TempoExtractionPreview::begin() const { return m_input.startSeconds + m_plan.audioShiftMsec / 1000; }
    double TempoExtractionPreview::end() const { return begin() + m_input.durationSeconds; }
    double TempoExtractionPreview::viewStart() const { return m_viewStart; }
    double TempoExtractionPreview::viewEnd() const { return m_viewEnd; }
    QValidator *TempoExtractionPreview::startTimeValidator() const { return m_startValidator; }
    QValidator *TempoExtractionPreview::endTimeValidator() const { return m_endValidator; }
    QValidator *TempoExtractionPreview::timeValidator() const { return m_timeValidator; }
    SVS::WaveformMipmap TempoExtractionPreview::waveform() const { return m_waveform; }
    double TempoExtractionPreview::waveformRate() const { return m_waveformRate; }
    bool TempoExtractionPreview::playing() const { return m_playing; }
    double TempoExtractionPreview::playhead() const { return m_playhead; }
    bool TempoExtractionPreview::metronome() const { return m_metronome; }
    bool TempoExtractionPreview::loop() const { return m_loop; }
    const TempoExtractionInput &TempoExtractionPreview::input() const { return m_input; }
    const TempoExtractionPlan &TempoExtractionPreview::plan() const { return m_plan; }

    void TempoExtractionPreview::setMetronome(bool value) {
        if (m_metronome == value)
            return;
        pause();
        m_metronome = value;
        Q_EMIT playbackChanged();
    }

    void TempoExtractionPreview::setLoop(bool value) {
        if (m_loop == value)
            return;
        pause();
        m_loop = value;
        Q_EMIT playbackChanged();
    }

    void TempoExtractionPreview::setRange(double start, double finish) {
        if (!std::isfinite(start) || !std::isfinite(finish))
            return;
        const auto minimum = std::min(0.001, end() - begin());
        start = std::clamp(start, begin(), end() - minimum);
        finish = std::clamp(finish, start + minimum, end());
        if (start == m_viewStart && finish == m_viewEnd)
            return;
        pause();
        m_viewStart = start;
        m_viewEnd = finish;
        m_playhead = std::clamp(m_playhead, start, finish);
        m_startValidator->setTop(SVS::LongTime(qRound(finish * 1000) - 1));
        m_endValidator->setBottom(SVS::LongTime(qRound(start * 1000) + 1));
        Q_EMIT rangeChanged();
        Q_EMIT playbackChanged();
    }

    void TempoExtractionPreview::selectBeat(double seconds, double radius) {
        if (!std::isfinite(seconds) || !std::isfinite(radius) || radius <= 0 || m_input.beats.isEmpty())
            return;
        const auto localSeconds = seconds - begin();
        auto it = std::lower_bound(m_input.beats.cbegin(), m_input.beats.cend(), localSeconds, [](const ExtractionBeat &beat, double time) { return beat.seconds < time; });
        if (it == m_input.beats.cend() || (it != m_input.beats.cbegin() && localSeconds - std::prev(it)->seconds < it->seconds - localSeconds))
            --it;
        if (std::abs(it->seconds - localSeconds) <= radius) {
            setReferenceBeat(static_cast<int>(it - m_input.beats.cbegin()));
            seek(begin() + it->seconds);
        }
    }

    QString TempoExtractionPreview::formatTime(int msec) const { return SVS::LongTime(msec).toString(); }

    int TempoExtractionPreview::parseTime(const QString &text, int fallback) const {
        bool ok;
        const auto time = SVS::LongTime::fromString(text, &ok);
        return ok ? time.totalMillisecond() : fallback;
    }

    double TempoExtractionPreview::tempoAt(double seconds) const {
        const auto &points = m_plan.tempoPoints;
        if (!std::isfinite(seconds) || points.isEmpty())
            return 0;
        const auto it = std::upper_bound(points.cbegin(), points.cend(), seconds, [](double time, const QPointF &point) { return time < point.x(); });
        return (it == points.cbegin() ? it : std::prev(it))->y();
    }

    void TempoExtractionPreview::schedule() {
        pause();
        if (m_cancelled)
            m_cancelled->store(true);
        ++m_revision;
        m_busy = true;
        Q_EMIT optionsChanged();
        Q_EMIT changed();
        m_calculationTimer.start();
    }

    void TempoExtractionPreview::calculate() {
        const auto revision = ++m_revision;
        m_cancelled = std::make_shared<std::atomic_bool>(false);
        auto watcher = new QFutureWatcher<TempoExtractionPlan>(this);
        connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, revision] {
            watcher->deleteLater();
            if (revision != m_revision)
                return;
            const auto previousBegin = begin();
            m_plan = watcher->result();
            const auto shift = begin() - previousBegin;
            m_viewStart += shift;
            m_viewEnd += shift;
            m_playhead += shift;
            m_startValidator->setRange(SVS::LongTime(qRound(begin() * 1000)), SVS::LongTime(qRound(m_viewEnd * 1000) - 1));
            m_endValidator->setRange(SVS::LongTime(qRound(m_viewStart * 1000) + 1), SVS::LongTime(qRound(end() * 1000)));
            m_timeValidator->setRange(SVS::LongTime(qRound(begin() * 1000)), SVS::LongTime(qRound(end() * 1000)));
            m_busy = false;
            if (!canKeepAudioPosition())
                m_options.moveAudio = true;
            if (m_options.automaticReference && m_plan.error.isEmpty() && !m_plan.tempi.isEmpty())
                m_options.referenceTick = m_plan.referenceTick;
            if (shift != 0)
                Q_EMIT timeOriginChanged();
            Q_EMIT rangeChanged();
            Q_EMIT playbackChanged();
            Q_EMIT optionsChanged();
            Q_EMIT changed();
        });
        watcher->setFuture(QtConcurrent::run(planExtractedTempo, m_input, m_options, m_cancelled));
    }

    void TempoExtractionPreview::loadWaveform() {
        struct WaveformResult {
            SVS::WaveformMipmap mipmap;
            double sampleRate{};
            QString error;
        };
        auto io = std::unique_ptr<talcs::AbstractAudioFormatIO>(Audio::GlobalAudioContext::formatManager()->getFormatLoad(m_audio.filePath, m_audio.userData, m_audio.formatEntryClassName));
        auto watcher = new QFutureWatcher<WaveformResult>(this);
        connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
            const auto result = watcher->result();
            watcher->deleteLater();
            m_waveform = result.mipmap;
            m_waveformRate = result.sampleRate;
            m_audioError = result.error;
            Q_EMIT waveformChanged();
            Q_EMIT changed();
        });
        watcher->setFuture(QtConcurrent::run([io = std::move(io), audio = m_audio, cancelled = m_waveformCancelled]() mutable {
            WaveformResult result;
            const auto fail = [&] {
                result.error = tr("The audio waveform could not be loaded.");
                return result;
            };
            if (!io || !io->open(talcs::AbstractAudioFormatIO::Read))
                return fail();
            const auto rate = io->sampleRate();
            const auto channels = io->channelCount();
            if (!std::isfinite(rate) || rate <= 0 || channels <= 0 || channels > 1024 || audio.durationSeconds * rate >= std::numeric_limits<qsizetype>::max())
                return fail();
            const auto length = qRound64(audio.durationSeconds * rate);
            const auto offset = qRound64(audio.offsetSeconds * rate);
            if (length <= 0 || offset < 0 || io->seek(offset) != offset)
                return fail();
            result.mipmap = SVS::WaveformMipmap(length, SVS::WaveformMipmap::Downscale, SVS::WaveformMipmap::Int16, false);
            QVector<float> interleaved(8192 * channels);
            QVector<float> mono(8192);
            for (qint64 position = 0; position < length && !cancelled->load();) {
                const auto count = std::min(qint64(8192), length - position);
                const auto read = io->read(interleaved.data(), count);
                if (read < 0 || read > count)
                    return fail();
                for (qint64 frame = 0; frame < count; ++frame) {
                    float value = 0;
                    if (frame < read) {
                        for (int channel = 0; channel < channels; ++channel) {
                            const auto sample = interleaved[frame * channels + channel];
                            if (std::isfinite(sample))
                                value += sample / channels;
                        }
                    }
                    mono[frame] = value;
                }
                result.mipmap.load(mono.constData(), 0, count, position);
                position += count;
            }
            io->close();
            result.sampleRate = rate;
            return result;
        }));
    }

    void TempoExtractionPreview::togglePlayback() {
        if (m_playing) {
            pause();
            return;
        }
        if (!valid())
            return;
        if (auto context = Audio::ProjectAudioContext::of(m_window); context && context->status() == Audio::ProjectAudioContext::Playing)
            context->setStatus(Audio::ProjectAudioContext::Paused);
        auto mixer = Audio::GlobalAudioContext::preMixer();
        if (!mixer || !mixer->isOpen() || Audio::GlobalAudioContext::sampleRate() <= 0) {
            m_audioError = tr("The audio output is unavailable.");
            Q_EMIT changed();
            return;
        }
        if (!m_source) {
            auto io = Audio::GlobalAudioContext::formatManager()->getFormatLoad(m_audio.filePath, m_audio.userData, m_audio.formatEntryClassName);
            if (!io) {
                m_audioError = tr("The audio file could not be opened for preview.");
                Q_EMIT changed();
                return;
            }
            m_source = std::make_unique<TempoExtractionAudioSource>(io, m_audio.offsetSeconds, m_audio.durationSeconds);
            m_transport = std::make_unique<talcs::TransportAudioSource>(m_source.get());
        }
        m_source->setBeats(m_plan.candidateBeats, begin());
        m_source->setMetronomeEnabled(m_metronome);
        m_source->setPlaybackEnd(m_viewEnd - begin());
        if (!mixer->addSource(m_transport.get())) {
            m_audioError = tr("The audio preview could not be started.");
            Q_EMIT changed();
            return;
        }
        const auto rate = Audio::GlobalAudioContext::sampleRate();
        m_outputRate = rate;
        if (m_playhead < m_viewStart || m_playhead >= m_viewEnd)
            m_playhead = m_viewStart;
        m_transport->setPosition(qRound64((m_playhead - begin()) * rate));
        m_transport->setLoopingRange(m_loop ? qRound64((m_viewStart - begin()) * rate) : -1, m_loop ? qRound64((m_viewEnd - begin()) * rate) : -1);
        m_transport->play();
        m_playing = true;
        m_playbackTimer.start();
        Q_EMIT playbackChanged();
    }

    void TempoExtractionPreview::pause() {
        if (m_transport && m_playing) {
            Audio::GlobalAudioContext::preMixer()->removeSource(m_transport.get());
            m_transport->pause();
            const auto rate = m_outputRate;
            if (rate > 0)
                m_playhead = std::clamp(begin() + m_transport->position() / rate, begin(), end());
            m_transport->close();
        }
        m_playing = false;
        m_playbackTimer.stop();
        Q_EMIT playbackChanged();
    }

    void TempoExtractionPreview::seek(double seconds) {
        if (!std::isfinite(seconds))
            return;
        const bool resume = m_playing;
        pause();
        m_playhead = std::clamp(seconds, m_viewStart, m_viewEnd);
        Q_EMIT playbackChanged();
        if (resume)
            togglePlayback();
    }

    void TempoExtractionPreview::releasePlayback() {
        pause();
        m_transport.reset();
        m_source.reset();
    }

}

#include "moc_TempoExtractionPreview.cpp"
