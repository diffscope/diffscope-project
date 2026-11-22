// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioMipmapAddOn.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include <QLoggingCategory>
#include <QPointer>
#include <QRunnable>
#include <QVector>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>

#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <audio/AudioClipAudioContext.h>
#include <audio/GlobalAudioContext.h>

#include <dspxproject/Document.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audiovisualizer/internal/AudioThumbnailWaveformThumbnail.h>

namespace AudioVisualizer::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioMipmapAddOn, "diffscope.audiovisualizer.audiomipmapaddon")

    AudioMipmapAddOn::AudioMipmapAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
        qRegisterMetaType<SVS::WaveformMipmap>();
    }

    AudioMipmapAddOn::~AudioMipmapAddOn() {
        m_closing = true;
        m_mipmapPool.clear();
        m_mipmapPool.waitForDone();
    }

    void AudioMipmapAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);

        auto trackList = windowInterface->projectDocumentContext()->document()->model()->tracks();
        const auto tracks = trackList->items();
        for (auto track : tracks) {
            for (auto clip : track->clips()->asRange()) {
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
            for (auto clip : track->clips()->asRange()) {
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
            for (auto clip : track->clips()->asRange()) {
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
        if (!clip || clip->kind() != dspx::Clip::Audio) {
            return;
        }
        auto audioClip = static_cast<dspx::AudioClip *>(clip);
        if (!m_audioClipThumbnailViews.contains(audioClip)) {
            m_audioClipThumbnailViews.insert(audioClip, {});
        }
        if (auto context = Audio::AudioClipAudioContext::of(audioClip)) {
            disconnect(context, &Audio::AudioClipAudioContext::audioChanged, this, nullptr);
            connect(context, &Audio::AudioClipAudioContext::audioChanged, this, [this, audioClip] {
                reloadAudioClip(audioClip);
            });
        }
        loadAudioClip(audioClip);
    }

    void AudioMipmapAddOn::removeClip(dspx::Clip *clip) {
        if (!clip || clip->kind() != dspx::Clip::Audio) {
            return;
        }
        auto audioClip = static_cast<dspx::AudioClip *>(clip);
        if (auto context = Audio::AudioClipAudioContext::of(audioClip)) {
            disconnect(context, &Audio::AudioClipAudioContext::audioChanged, this, nullptr);
        }
        m_audioClipSampleRates.remove(audioClip);
        m_mipmapRequests.remove(audioClip);
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
        m_mipmapRequests.remove(clip);
        auto context = Audio::AudioClipAudioContext::of(clip);
        if (!context) {
            qCWarning(lcAudioMipmapAddOn) << "Audio clip context not found:" << clip;
            processAudioClipMipmap(clip, 0, {});
            return;
        }
        const auto filePath = context->realAudioPath();
        if (filePath.isEmpty()) {
            processAudioClipMipmap(clip, 0, {});
            return;
        }
        const auto request = ++m_nextMipmapRequest;
        m_mipmapRequests.insert(clip, request);
        QPointer<AudioMipmapAddOn> that(this);
        const auto path = clip->path();
        m_mipmapPool.start(QRunnable::create([that, clip, request, filePath, path] {
            std::unique_ptr<talcs::AbstractAudioFormatIO> io(
                Audio::GlobalAudioContext::formatManager()->getFormatLoad(filePath, path.userData, path.formatEntryClassName));
            if (io && !io->openMode() && !io->open(talcs::AbstractAudioFormatIO::Read)) {
                io.reset();
            }
            const auto rate = io ? io->sampleRate() : 0;
            const auto channels = io ? io->channelCount() : 0;
            const auto length = io ? io->length() : 0;
            SVS::WaveformMipmap mipmap;
            if (std::isfinite(rate) && rate > 0 && length > 0 && length <= std::numeric_limits<qsizetype>::max() && channels > 0 && channels <= 4096) {
                mipmap = SVS::WaveformMipmap(length, SVS::WaveformMipmap::Downscale, SVS::WaveformMipmap::Int16, false);
                const auto chunk = 65536 / channels / 16 * 16;
                QVector<float> frames(chunk * channels);
                QVector<float> mono(chunk);
                for (qint64 position = 0; position < length && !that->m_closing;) {
                    const auto count = std::min<qint64>(length - position, chunk);
                    if (io->read(frames.data(), count) != count) {
                        mipmap = {};
                        break;
                    }
                    for (qint64 frame = 0; frame < count; ++frame) {
                        float sum = 0;
                        for (int channel = 0; channel < channels; ++channel) {
                            const auto sample = frames[frame * channels + channel];
                            if (std::isfinite(sample)) {
                                sum += sample / channels;
                            }
                        }
                        mono[frame] = sum;
                    }
                    mipmap.load(mono.constData(), 0, count, position);
                    position += count;
                }
            }
            if (!that->m_closing) {
                QMetaObject::invokeMethod(that.data(), [that, clip, request, rate, mipmap] {
                    if (that && that->m_mipmapRequests.value(clip) == request) {
                        that->processAudioClipMipmap(clip, rate, mipmap);
                    }
                }, Qt::QueuedConnection);
            }
        }));
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
