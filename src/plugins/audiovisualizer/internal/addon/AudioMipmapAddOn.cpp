#include "AudioMipmapAddOn.h"

#include <QLoggingCategory>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <QVector>

#include <memory>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>

#include <dspxmodel/AudioClip.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>

#include <audio/AudioClipAudioContext.h>
#include <audio/GlobalAudioContext.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audiovisualizer/internal/AudioThumbnailWaveformThumbnail.h>

namespace AudioVisualizer::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioMipmapAddOn, "diffscope.audiovisualizer.audiomipmapaddon")

    AudioMipmapAddOn::AudioMipmapAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
        qRegisterMetaType<SVS::WaveformMipmap>();
    }

    AudioMipmapAddOn::~AudioMipmapAddOn() = default;

    void AudioMipmapAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);

        connect(this, &AudioMipmapAddOn::mipmapLoaded, this, &AudioMipmapAddOn::handleMipmapLoaded);

        auto trackList = windowInterface->projectDocumentContext()->document()->model()->tracks();
        const auto tracks = trackList->items();
        for (auto track : tracks) {
            const auto clips = track->clips()->slice(0, track->clips()->size());
            for (auto clip : clips) {
                addClip(clip);
            }
            connect(track->clips(), &dspx::ClipSequence::itemInserted, this, [this](dspx::Clip *clip) {
                addClip(clip);
            });
            connect(track->clips(), &dspx::ClipSequence::itemRemoved, this, [this](dspx::Clip *clip) {
                removeClip(clip);
            });
        }
        connect(trackList, &dspx::TrackList::itemInserted, this, [this](int index, dspx::Track *track) {
            Q_UNUSED(index)
            const auto clips = track->clips()->slice(0, track->clips()->size());
            for (auto clip : clips) {
                addClip(clip);
            }
            connect(track->clips(), &dspx::ClipSequence::itemInserted, this, [this](dspx::Clip *clip) {
                addClip(clip);
            });
            connect(track->clips(), &dspx::ClipSequence::itemRemoved, this, [this](dspx::Clip *clip) {
                removeClip(clip);
            });
        });
        connect(trackList, &dspx::TrackList::itemRemoved, this, [this](int index, dspx::Track *track) {
            Q_UNUSED(index)
            const auto clips = track->clips()->slice(0, track->clips()->size());
            for (auto clip : clips) {
                removeClip(clip);
            }
        });
    }

    void AudioMipmapAddOn::extensionsInitialized() {
    }

    bool AudioMipmapAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    AudioMipmapAddOn *AudioMipmapAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<AudioMipmapAddOn>();
    }

    void AudioMipmapAddOn::mapThumbnailView(dspx::AudioClip *clip, AudioThumbnailWaveformThumbnail *view) {
        auto it = m_audioClipThumbnailViews.find(clip);
        Q_ASSERT(view);
        if (it != m_audioClipThumbnailViews.end()) {
            it->insert(view);
        }
        if (view) {
            view->setSampleRate(m_audioClipSampleRates.value(clip, 0.0));
            view->setWaveformMipmap(m_audioClipMipmaps.value(clip));
        }
    }

    void AudioMipmapAddOn::unmapThumbnailVIew(dspx::AudioClip *clip, AudioThumbnailWaveformThumbnail *view) {
        auto it = m_audioClipThumbnailViews.find(clip);
        Q_ASSERT(view);
        if (it != m_audioClipThumbnailViews.end()) {
            it->remove(view);
        }
    }

    void AudioMipmapAddOn::addClip(dspx::Clip *clip) {
        if (!clip || clip->type() != dspx::Clip::Audio) {
            return;
        }
        auto audioClip = static_cast<dspx::AudioClip *>(clip);
        if (!m_audioClipThumbnailViews.contains(audioClip)) {
            m_audioClipThumbnailViews.insert(audioClip, {});
        }
        disconnect(audioClip, &dspx::AudioClip::pathChanged, this, nullptr);
        connect(audioClip, &dspx::AudioClip::pathChanged, this, [this, audioClip] {
            reloadAudioClip(audioClip);
        });
        loadAudioClip(audioClip);
    }

    void AudioMipmapAddOn::removeClip(dspx::Clip *clip) {
        if (!clip || clip->type() != dspx::Clip::Audio) {
            return;
        }
        auto audioClip = static_cast<dspx::AudioClip *>(clip);
        disconnect(audioClip, &dspx::AudioClip::pathChanged, this, nullptr);
        m_audioClipSampleRates.remove(audioClip);
        m_audioClipMipmaps.remove(audioClip);
        m_audioClipThumbnailViews.remove(audioClip);
    }

    void AudioMipmapAddOn::reloadAudioClip(dspx::AudioClip *clip) {
        m_audioClipSampleRates.remove(clip);
        m_audioClipMipmaps.remove(clip);
        processAudioClipMipmap(clip, 0, {});
        loadAudioClip(clip);
    }

    void AudioMipmapAddOn::loadAudioClip(dspx::AudioClip *clip) {
        auto context = Audio::AudioClipAudioContext::of(clip);
        if (!context) {
            qCWarning(lcAudioMipmapAddOn) << "Audio clip context not found:" << clip;
            processAudioClipMipmap(clip, 0, {});
            return;
        }
        if (context->realAudioPath().isEmpty()) {
            processAudioClipMipmap(clip, 0, {});
            return;
        }

        const auto filePath = context->realAudioPath();

        qCInfo(lcAudioMipmapAddOn) << "Loading audio clip:" << clip << filePath;

        const auto path = clip->path();
        auto io = std::unique_ptr<talcs::AbstractAudioFormatIO>(
            Audio::GlobalAudioContext::formatManager()->getFormatLoad(filePath, path.userData, path.formatEntryClassName));
        if (!io) {
            qCWarning(lcAudioMipmapAddOn) << "Failed to create audio format IO:" << filePath;
            processAudioClipMipmap(clip, 0, {});
            return;
        }
        if (!io->open(talcs::AbstractAudioFormatIO::Read)) {
            qCWarning(lcAudioMipmapAddOn) << "Failed to open audio format IO:" << filePath;
            processAudioClipMipmap(clip, 0, {});
            return;
        }

        const auto sampleRate = io->sampleRate();
        const auto length = io->length();
        const auto channelCount = io->channelCount();
        if (sampleRate <= 0 || length < 0 || channelCount <= 0) {
            qCWarning(lcAudioMipmapAddOn) << "Invalid audio metadata:" << filePath << sampleRate << length << channelCount;
            io->close();
            processAudioClipMipmap(clip, 0, {});
            return;
        }

        QVector<float> rawData(length * channelCount);
        const auto readLength = length == 0 ? 0 : io->read(rawData.data(), length);
        io->close();
        if (readLength != length) {
            qCWarning(lcAudioMipmapAddOn) << "Failed to read full audio data:" << filePath << readLength << length;
            processAudioClipMipmap(clip, 0, {});
            return;
        }

        QVector<float> audioData(length);
        if (channelCount == 1) {
            audioData = std::move(rawData);
        } else {
            for (qint64 i = 0; i < length; ++i) {
                float sum = 0;
                for (int ch = 0; ch < channelCount; ++ch) {
                    sum += rawData.at(i * channelCount + ch);
                }
                audioData[i] = sum / channelCount;
            }
        }

        m_audioClipSampleRates.insert(clip, sampleRate);
        QPointer<AudioMipmapAddOn> that(this);
        QThreadPool::globalInstance()->start(QRunnable::create([that, clip, sampleRate, audioData = std::move(audioData)]() mutable {
            SVS::WaveformMipmap mipmap(audioData.size(), SVS::WaveformMipmap::Downscale, SVS::WaveformMipmap::Int16, false);
            mipmap.load(audioData.constData(), 0, audioData.size(), 0);
            if (that) {
                emit that->mipmapLoaded(clip, sampleRate, mipmap);
            }
        }));
    }

    void AudioMipmapAddOn::handleMipmapLoaded(dspx::AudioClip *clip, double sampleRate, const SVS::WaveformMipmap &mipmap) {
        if (!m_audioClipSampleRates.contains(clip)) {
            return;
        }
        processAudioClipMipmap(clip, sampleRate, mipmap);
    }

    void AudioMipmapAddOn::processAudioClipMipmap(dspx::AudioClip *clip, double sampleRate, const SVS::WaveformMipmap &mipmap) {
        if (sampleRate > 0 && mipmap.isValid()) {
            m_audioClipSampleRates.insert(clip, sampleRate);
            m_audioClipMipmaps.insert(clip, mipmap);
        } else {
            m_audioClipSampleRates.remove(clip);
            m_audioClipMipmaps.remove(clip);
        }
        const auto views = m_audioClipThumbnailViews.value(clip);
        for (auto view : views) {
            if (!view) {
                continue;
            }
            view->setSampleRate(sampleRate);
            view->setWaveformMipmap(mipmap);
        }
        qCInfo(lcAudioMipmapAddOn) << "Mipmap loaded:" << clip << sampleRate << mipmap.isValid();

        // TODO: Notify waveform consumers after the thumbnail pipeline is wired.
    }

}
