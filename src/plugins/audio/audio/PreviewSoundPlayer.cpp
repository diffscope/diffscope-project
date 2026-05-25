#include "PreviewSoundPlayer.h"
#include "PreviewSoundPlayer_p.h"

#include <QMetaObject>
#include <QPointer>

#include <atomic>

#include <TalcsCore/AudioSource.h>
#include <TalcsCore/MixerAudioSource.h>
#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/AudioFormatInputSource.h>

#include <audio/GlobalAudioContext.h>

namespace Audio {

    class PreviewSoundFinishedFilter final : public talcs::AudioSource {
    public:
        PreviewSoundFinishedFilter(PreviewSoundPlayer *player, PreviewSoundPlayerPrivate *playerPrivate,
                                   talcs::AudioFormatInputSource *source, quint64 serial)
            : m_player(player), m_playerPrivate(playerPrivate), m_source(source), m_serial(serial) {
        }

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override {
            if (m_finished.load()) {
                return readData.length;
            }

            if (!m_source || m_source->nextReadPosition() < m_source->length()) {
                return readData.length;
            }

            if (!m_finished.exchange(true)) {
                auto player = m_player;
                auto playerPrivate = m_playerPrivate;
                auto source = m_source;
                auto serial = m_serial;
                QMetaObject::invokeMethod(player, [playerPrivate, source, serial] {
                    playerPrivate->finish(source, serial);
                }, Qt::QueuedConnection);
            }
            return readData.length;
        }

    private:
        PreviewSoundPlayer *m_player{};
        PreviewSoundPlayerPrivate *m_playerPrivate{};
        talcs::AudioFormatInputSource *m_source{};
        quint64 m_serial{};
        std::atomic_bool m_finished{false};
    };

    PreviewSoundPlayerPrivate::PreviewSoundPlayerPrivate(talcs::AbstractAudioFormatIO *audioFormatIo)
        : audioFormatIo(audioFormatIo) {
    }

    PreviewSoundPlayerPrivate::~PreviewSoundPlayerPrivate() = default;

    void PreviewSoundPlayerPrivate::destroySource(talcs::AudioFormatInputSource *expectedSource) {
        if (!source || (expectedSource && source.get() != expectedSource)) {
            return;
        }

        auto oldSource = std::move(source);
        auto oldFilter = std::move(finishedFilter);
        ++serial;

        GlobalAudioContext::preMixer()->removeSource(oldSource.get());
        oldSource->close();
        oldSource->setReadingFilter(nullptr);
        oldFilter.reset();
    }

    void PreviewSoundPlayerPrivate::finish(talcs::AudioFormatInputSource *source, quint64 serial) {
        Q_Q(PreviewSoundPlayer);
        if (this->serial != serial || this->source.get() != source) {
            return;
        }

        QPointer<PreviewSoundPlayer> guard(q);
        Q_EMIT q->finished();
        if (!guard) {
            return;
        }
        destroySource(source);
    }

    PreviewSoundPlayer::PreviewSoundPlayer(talcs::AbstractAudioFormatIO *audioFormatIo, QObject *parent)
        : QObject(parent), d_ptr(new PreviewSoundPlayerPrivate(audioFormatIo)) {
        Q_D(PreviewSoundPlayer);
        d->q_ptr = this;
    }

    PreviewSoundPlayer::~PreviewSoundPlayer() {
        stop();
    }

    void PreviewSoundPlayer::play() {
        Q_D(PreviewSoundPlayer);
        d->destroySource();
        if (!d->audioFormatIo) {
            return;
        }

        auto source = std::make_unique<talcs::AudioFormatInputSource>(d->audioFormatIo.get(), false);
        source->setNextReadPosition(0);

        const auto serial = ++d->serial;
        auto filter = std::make_unique<PreviewSoundFinishedFilter>(this, d, source.get(), serial);
        source->setReadingFilter(filter.get());

        if (!GlobalAudioContext::preMixer()->addSource(source.get(), false)) {
            source->close();
            source->setReadingFilter(nullptr);
            return;
        }

        d->finishedFilter = std::move(filter);
        d->source = std::move(source);
    }

    void PreviewSoundPlayer::stop() {
        Q_D(PreviewSoundPlayer);
        d->destroySource();
    }

}

#include "moc_PreviewSoundPlayer.cpp"
