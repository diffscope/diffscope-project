// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PreviewSoundPlayer.h"
#include "PreviewSoundPlayer_p.h"

#include <atomic>

#include <QMetaObject>
#include <QPointer>
#include <QTimer>

#include <TalcsCore/AudioSource.h>
#include <TalcsCore/MixerAudioSource.h>
#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/AudioFormatInputSource.h>

#include <audio/GlobalAudioContext.h>

namespace Audio {

    // An AudioFormatInputSource whose position is tracked in seconds, so that the playhead stays at the same
    // time offset when the pre-mixer is reopened with a different sample rate.
    class PreviewSoundSource : public talcs::AudioFormatInputSource {
    public:
        using AudioFormatInputSource::AudioFormatInputSource;

        // The sample rate of the output domain the persisted position belongs to, or 0 if never opened.
        double domainSampleRate() const {
            if (const auto rate = sampleRate(); rate > 0) {
                return rate;
            }
            return m_lastOpenedSampleRate.load();
        }

        double positionSecond() const {
            const auto rate = domainSampleRate();
            if (rate <= 0) {
                return m_pendingPositionSecond >= 0 ? m_pendingPositionSecond : 0.0;
            }
            return nextReadPosition() / rate;
        }

        void setPositionSecond(double positionSecond) {
            const auto rate = domainSampleRate();
            if (rate <= 0) {
                m_pendingPositionSecond = positionSecond;
                return;
            }
            setNextReadPosition(qRound64(positionSecond * rate));
        }

        bool open(qint64 bufferSize, double sampleRate) override {
            // AudioFormatInputSource persists the position in output samples, whose time offset would drift
            // if the output sample rate changed. Convert it here to keep the time position stable.
            if (m_pendingPositionSecond >= 0) {
                // The source has never been opened; setNextReadPosition only stores the value and the base
                // open() below picks it up for the initial IO seek.
                setNextReadPosition(qRound64(m_pendingPositionSecond * sampleRate));
                m_pendingPositionSecond = -1;
            } else {
                const auto lastOpenedSampleRate = m_lastOpenedSampleRate.load();
                if (lastOpenedSampleRate > 0 && !qFuzzyCompare(lastOpenedSampleRate, sampleRate)) {
                    setNextReadPosition(qRound64(nextReadPosition() / lastOpenedSampleRate * sampleRate));
                }
            }
            const bool ok = AudioFormatInputSource::open(bufferSize, sampleRate);
            if (ok) {
                m_lastOpenedSampleRate = sampleRate;
            }
            return ok;
        }

        void close() override {
            AudioFormatInputSource::close();
            // m_lastOpenedSampleRate is kept so that reopening with a different sample rate (after the
            // device has been closed in between) still converts the persisted position correctly.
        }

    private:
        std::atomic<double> m_lastOpenedSampleRate{};
        double m_pendingPositionSecond{-1};
    };

    class PreviewSoundEndFilter final : public talcs::AudioSource {
    public:
        PreviewSoundEndFilter(PreviewSoundPlayer *player, PreviewSoundPlayerPrivate *playerPrivate,
                          PreviewSoundSource *source, quint64 serial)
            : m_player(player), m_playerPrivate(playerPrivate), m_source(source), m_serial(serial) {
        }

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override {
            if (m_finished.load()) {
                return readData.length;
            }

            if (!m_source || !m_source->isOpen() || m_source->nextReadPosition() < m_source->length()) {
                return readData.length;
            }

            if (!m_finished.exchange(true)) {
                auto player = m_player;
                auto playerPrivate = m_playerPrivate;
                auto serial = m_serial;
                QMetaObject::invokeMethod(player, [playerPrivate, serial] {
                    playerPrivate->handleFinished(serial);
                }, Qt::QueuedConnection);
            }
            return readData.length;
        }

    private:
        PreviewSoundPlayer *m_player{};
        PreviewSoundPlayerPrivate *m_playerPrivate{};
        PreviewSoundSource *m_source{};
        quint64 m_serial{};
        std::atomic_bool m_finished{false};
    };

    PreviewSoundPlayerPrivate::PreviewSoundPlayerPrivate(talcs::AbstractAudioFormatIO *audioFormatIo, bool takeOwnership)
        : audioFormatIo(audioFormatIo, takeOwnership) {
        // The length and sample rate of the IO are only available when it is open, so fetch them eagerly.
        // They never change afterwards because the IO is never replaced.
        if (audioFormatIo && audioFormatIo->open(talcs::AbstractAudioFormatIO::Read)) {
            if (const auto ioSampleRate = audioFormatIo->sampleRate(); ioSampleRate > 0) {
                lengthSecond = audioFormatIo->length() / ioSampleRate;
            }
            audioFormatIo->close();
        }
    }

    void PreviewSoundPlayerPrivate::start() {
        Q_Q(PreviewSoundPlayer);
        if (playing || lengthSecond <= 0) {
            return;
        }

        if (!source) {
            source = std::make_unique<PreviewSoundSource>(audioFormatIo.get(), false);
        }
        source->setPositionSecond(positionSecond);

        ++serial;
        endFilter = std::make_unique<PreviewSoundEndFilter>(q, this, source.get(), serial);
        source->setReadingFilter(endFilter.get());

        if (!GlobalAudioContext::preMixer()->addSource(source.get(), false)) {
            source->close();
            source->setReadingFilter(nullptr);
            endFilter.reset();
            return;
        }

        if (!positionTimer) {
            positionTimer = new QTimer(q);
            positionTimer->setInterval(50);
            QObject::connect(positionTimer, &QTimer::timeout, q, [this] {
                syncPositionFromSource();
            });
        }
        positionTimer->start();

        playing = true;
        Q_EMIT q->playingChanged(true);
        syncPositionFromSource();
    }

    void PreviewSoundPlayerPrivate::pause() {
        Q_Q(PreviewSoundPlayer);
        if (!playing) {
            return;
        }
        positionTimer->stop();
        ++serial;
        // Capture the final position while the source is still open
        auto finalPosition = positionSecond;
        if (source) {
            if (source->isOpen() && source->sampleRate() > 0) {
                finalPosition = qBound(0.0, source->positionSecond(), lengthSecond);
            }
            GlobalAudioContext::preMixer()->removeSource(source.get());
            source->close();
            source->setReadingFilter(nullptr);
        }
        endFilter.reset();
        playing = false;
        QPointer<PreviewSoundPlayer> guard(q);
        if (!qFuzzyCompare(finalPosition + 1, positionSecond + 1)) {
            positionSecond = finalPosition;
            Q_EMIT q->positionSecondChanged(finalPosition);
            // The emission above might have re-entered the state machine or destroyed the object
            if (!guard || playing) {
                return;
            }
        }
        Q_EMIT q->playingChanged(false);
    }

    void PreviewSoundPlayerPrivate::handleFinished(quint64 serial) {
        Q_Q(PreviewSoundPlayer);
        if (!playing || this->serial != serial) {
            return;
        }
        positionTimer->stop();
        ++this->serial;
        if (source) {
            GlobalAudioContext::preMixer()->removeSource(source.get());
            source->close();
            source->setReadingFilter(nullptr);
        }
        endFilter.reset();
        playing = false;
        QPointer<PreviewSoundPlayer> guard(q);
        // Keep the position at the end instead of resetting it
        if (!qFuzzyCompare(positionSecond + 1, lengthSecond + 1)) {
            positionSecond = lengthSecond;
            Q_EMIT q->positionSecondChanged(lengthSecond);
            // The emission above might have re-entered the state machine or destroyed the object
            if (!guard || playing) {
                return;
            }
        }
        Q_EMIT q->playingChanged(false);
    }

    void PreviewSoundPlayerPrivate::syncPositionFromSource() {
        Q_Q(PreviewSoundPlayer);
        // The source may be momentarily closed while the pre-mixer is reopened (e.g. a device change);
        // skip such ticks to keep the reported position continuous.
        if (!source || !source->isOpen() || source->sampleRate() <= 0) {
            return;
        }
        const auto position = qBound(0.0, source->positionSecond(), lengthSecond);
        if (qFuzzyCompare(position + 1, positionSecond + 1)) {
            return;
        }
        positionSecond = position;
        Q_EMIT q->positionSecondChanged(position);
    }

    PreviewSoundPlayer::PreviewSoundPlayer(talcs::AbstractAudioFormatIO *audioFormatIo, bool takeOwnership, QObject *parent)
        : QObject(parent), d_ptr(new PreviewSoundPlayerPrivate(audioFormatIo, takeOwnership)) {
        Q_D(PreviewSoundPlayer);
        d->q_ptr = this;
    }

    PreviewSoundPlayer::~PreviewSoundPlayer() {
        Q_D(PreviewSoundPlayer);
        ++d->serial;
        if (d->positionTimer) {
            d->positionTimer->stop();
        }
        if (d->source) {
            GlobalAudioContext::preMixer()->removeSource(d->source.get());
            d->source->close();
            d->source->setReadingFilter(nullptr);
        }
        d->endFilter.reset();
    }

    bool PreviewSoundPlayer::isPlaying() const {
        Q_D(const PreviewSoundPlayer);
        return d->playing;
    }

    void PreviewSoundPlayer::setPlaying(bool playing) {
        Q_D(PreviewSoundPlayer);
        if (playing) {
            d->start();
        } else {
            d->pause();
        }
    }

    double PreviewSoundPlayer::positionSecond() const {
        Q_D(const PreviewSoundPlayer);
        return d->positionSecond;
    }

    void PreviewSoundPlayer::setPositionSecond(double positionSecond) {
        Q_D(PreviewSoundPlayer);
        const auto position = qBound(0.0, positionSecond, d->lengthSecond);
        if (qFuzzyCompare(position + 1, d->positionSecond + 1)) {
            return;
        }
        d->positionSecond = position;
        if (d->source) {
            d->source->setPositionSecond(position);
        }
        Q_EMIT positionSecondChanged(position);
    }

    double PreviewSoundPlayer::lengthSecond() const {
        Q_D(const PreviewSoundPlayer);
        return d->lengthSecond;
    }

}

#include "moc_PreviewSoundPlayer.cpp"
