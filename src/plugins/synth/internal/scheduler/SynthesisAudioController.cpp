// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisAudioController.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include <QPromise>

#include <TalcsCore/FutureAudioSource.h>
#include <TalcsCore/FutureAudioSourceClipSeries.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsFormat/AudioFormatInputSource.h>
#include <TalcsFormat/FormatManager.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/TrackAudioContext.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <synth/SynthesisPiece.h>
#include <synth/internal/SynthesisProjectAddOn.h>
#include <synth/internal/SynthesisProjectInput.h>
#include <synth/private/SynthesisPiece_p.h>

namespace Synth::Internal {

    namespace {

        struct AudioClipRange {
            qint64 position{};
            qint64 sourceStart{};
            qint64 length{};

            bool isValid() const {
                return length > 0;
            }
        };

        AudioClipRange audioClipRange(Core::ProjectWindowInterface *window, Audio::TrackAudioContext *trackContext, dspx::SingingClip *clip, SynthesisPiece *piece, double sampleRate) {
            if (!window || !trackContext || !clip || !piece) {
                return {};
            }
            auto timeline = window->projectTimeline()->musicTimeline();
            const double pieceStartTick = clip->start() + piece->position();
            const double pieceEndTick = pieceStartTick + piece->length();
            const double visibleStartTick = clip->position();
            const double visibleEndTick = visibleStartTick + clip->clipLength();
            const double startTick = std::max(pieceStartTick, visibleStartTick);
            const double endTick = std::min(pieceEndTick, visibleEndTick);
            if (endTick <= startTick) {
                return {};
            }
            if (qFuzzyIsNull(sampleRate)) {
                sampleRate = trackContext->trackMixer()->sampleRate();
                if (qFuzzyIsNull(sampleRate)) {
                    sampleRate = Audio::GlobalAudioContext::sampleRate();
                }
            }
            if (qFuzzyIsNull(sampleRate)) {
                return {};
            }
            const double pieceStartSeconds = ProjectInput::tickSeconds(timeline, pieceStartTick);
            const double startSeconds = ProjectInput::tickSeconds(timeline, startTick);
            const double endSeconds = ProjectInput::tickSeconds(timeline, endTick);
            const auto pieceStartSample = static_cast<qint64>(std::llround(pieceStartSeconds * sampleRate));
            const auto startSample = static_cast<qint64>(std::llround(startSeconds * sampleRate));
            const auto endSample = static_cast<qint64>(std::llround(endSeconds * sampleRate));
            return {
                startSample,
                startSample - pieceStartSample,
                std::max<qint64>(1, endSample - startSample),
            };
        }

    }

    struct SynthesisAudioController::Binding {
        talcs::FutureAudioSourceClipSeries *series{};
        talcs::FutureAudioSourceClipSeries::ClipView clip;
        talcs::FutureAudioSource *futureSource{};
        std::shared_ptr<QPromise<talcs::PositionableAudioSource *>> promise;
        talcs::PositionableMixerAudioSource *mixer{};
        talcs::AudioFormatInputSource *source{};
        AudioClipRange range;
    };

    SynthesisAudioController::SynthesisAudioController(QObject *parent)
        : QObject(parent) {
    }

    SynthesisAudioController::~SynthesisAudioController() {
        const auto bindings = std::exchange(m_bindings, {});
        for (auto binding : bindings) {
            destroyBinding(binding);
        }
    }

    bool SynthesisAudioController::prepare(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, QPointer<Audio::TrackAudioContext> &trackContext, talcs::FutureAudioSourceClipSeries *&series, QString *errorMessage) {
        if (!clip || !piece || piece->singingClip() != clip) {
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("The synthesis piece is no longer available.");
            }
            return false;
        }
        auto clipSequence = clip->clipSequence();
        auto track = clipSequence ? clipSequence->track() : nullptr;
        auto currentTrackContext = track ? Audio::TrackAudioContext::of(track) : nullptr;
        auto projectAudioContext = Audio::ProjectAudioContext::of(window);
        if (!currentTrackContext || !projectAudioContext || !projectAudioContext->transport()) {
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("The project audio context is not available.");
            }
            return false;
        }

        if (series && trackContext != currentTrackContext) {
            detachSeries(trackContext, series);
        }
        if (!series) {
            series = new talcs::FutureAudioSourceClipSeries(this);
            series->setBufferingTarget(projectAudioContext->transport());
            series->setReadMode(talcs::FutureAudioSourceClipSeries::Notify);
            if (!currentTrackContext->trackMixer()->addSource(series)) {
                delete series;
                series = nullptr;
                if (errorMessage) {
                    *errorMessage = SynthesisProjectAddOn::tr("Synthesized audio for the synthesis piece could not be added to the track.");
                }
                return false;
            }
            trackContext = currentTrackContext;
        }

        const auto range = audioClipRange(window, currentTrackContext, clip, piece, 0.0);
        if (!range.isValid()) {
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("The synthesis piece is outside the visible range of its clip.");
            }
            return false;
        }
        if (auto binding = m_bindings.value(piece);
            binding && binding->series == series && binding->range.position == range.position &&
            binding->range.sourceStart == range.sourceStart && binding->range.length == range.length) {
            return true;
        }
        remove(piece);

        auto promise = std::make_shared<QPromise<talcs::PositionableAudioSource *>>();
        const qint64 contentLength = std::max<qint64>(1, range.sourceStart + range.length);
        const int progressMaximum = static_cast<int>(std::min<qint64>(contentLength, std::numeric_limits<int>::max()));
        promise->setProgressRange(0, progressMaximum);
        promise->start();
        auto futureSource = new talcs::FutureAudioSource(promise->future(), {}, this);
        connect(futureSource, &talcs::FutureAudioSource::statusChanged, this, [this, piece = QPointer<SynthesisPiece>(piece), futureSource = QPointer<talcs::FutureAudioSource>(futureSource)] {
            const auto binding = m_bindings.value(piece.data());
            if (piece && futureSource && binding && binding->futureSource == futureSource.data()) {
                Q_EMIT statusChanged(piece.data());
            }
        });
        const auto clipView = series->insertClip(futureSource, range.position, range.sourceStart, range.length);
        if (!clipView.isValid()) {
            futureSource->cancel();
            delete futureSource;
            promise->finish();
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("The synthesis piece overlaps another synthesis piece in the same clip.");
            }
            return false;
        }
        m_bindings.insert(piece, new Binding{
                                     series,
                                     clipView,
                                     futureSource,
                                     std::move(promise),
                                     nullptr,
                                     nullptr,
                                     range,
                                 });
        return true;
    }

    bool SynthesisAudioController::install(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, QPointer<Audio::TrackAudioContext> &trackContext, talcs::FutureAudioSourceClipSeries *&series, const QString &filePath, QString *errorMessage) {
        if (!prepare(window, clip, piece, trackContext, series, errorMessage)) {
            return false;
        }
        auto binding = m_bindings.value(piece);
        if (!binding || !binding->futureSource || !binding->promise || binding->futureSource->source()) {
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("The synthesis piece is no longer waiting for synthesized audio.");
            }
            return false;
        }
        auto io = Audio::GlobalAudioContext::formatManager()->getFormatLoad(filePath);
        if (!io) {
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("The synthesized audio file could not be opened.");
            }
            return false;
        }
        auto source = new talcs::AudioFormatInputSource(io, true);
        source->setStereoize(true);
        auto mixer = new talcs::PositionableMixerAudioSource;
        if (!mixer->addSource(source)) {
            delete mixer;
            delete source;
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("Synthesized audio for the synthesis piece could not be prepared.");
            }
            return false;
        }
        mixer->setGain(static_cast<float>(clip->gain()));
        mixer->setPan(static_cast<float>(clip->pan()));
        mixer->setSilentFlags(clip->mute() ? -1 : 0);
        connect(clip, &dspx::Clip::gainChanged, mixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(clip, &dspx::Clip::panChanged, mixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(clip, &dspx::Clip::muteChanged, mixer, [mixer](bool mute) {
            mixer->setSilentFlags(mute ? -1 : 0);
        });
        binding->mixer = mixer;
        binding->source = source;
        auto d = SynthesisPiecePrivate::get(piece);
        d->audioFilePath = filePath;
        d->errorMessage.clear();
        Q_EMIT piece->audioFileChanged();
        binding->promise->setProgressValue(binding->promise->future().progressMaximum());
        binding->promise->addResult(mixer);
        binding->promise->finish();
        return true;
    }

    bool SynthesisAudioController::refreshRange(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, Audio::TrackAudioContext *trackContext, talcs::FutureAudioSourceClipSeries *series, double sampleRate, QString *errorMessage) {
        const auto binding = m_bindings.value(piece);
        if (!binding) {
            return true;
        }
        if (!clip || !piece || piece->singingClip() != clip || !trackContext || !series || binding->series != series) {
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("The synthesis piece is no longer available.");
            }
            return false;
        }
        const auto range = audioClipRange(window, trackContext, clip, piece, sampleRate);
        if (!range.isValid()) {
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("The synthesis piece is outside the visible range of its clip.");
            }
            return false;
        }
        if (binding->range.position == range.position &&
            binding->range.sourceStart == range.sourceStart &&
            binding->range.length == range.length) {
            return true;
        }
        if (!series->setClipRange(binding->clip, range.position, range.length)) {
            if (errorMessage) {
                *errorMessage = SynthesisProjectAddOn::tr("Synthesized audio for the synthesis piece could not be repositioned.");
            }
            return false;
        }
        series->setClipStartPos(binding->clip, range.sourceStart);
        if (binding->promise && !binding->promise->future().isFinished()) {
            const qint64 contentLength = std::max<qint64>(1, range.sourceStart + range.length);
            const int progressMaximum = static_cast<int>(std::min<qint64>(contentLength, std::numeric_limits<int>::max()));
            binding->promise->setProgressRange(0, progressMaximum);
        }
        binding->range = range;
        return true;
    }

    void SynthesisAudioController::destroyBinding(Binding *binding) {
        if (!binding) {
            return;
        }
        if (binding->series && binding->clip.isValid()) {
            binding->series->removeClip(binding->clip);
        }
        if (binding->futureSource) {
            if (!binding->futureSource->source()) {
                binding->futureSource->cancel();
            }
            delete binding->futureSource;
        }
        if (binding->promise && !binding->promise->future().isFinished()) {
            binding->promise->finish();
        }
        if (binding->mixer && binding->source) {
            binding->mixer->removeSource(binding->source);
        }
        delete binding->mixer;
        delete binding->source;
        delete binding;
    }

    void SynthesisAudioController::remove(SynthesisPiece *piece) {
        if (!piece) {
            return;
        }
        destroyBinding(m_bindings.take(piece));
        auto d = SynthesisPiecePrivate::get(piece);
        if (!d->audioFilePath.isEmpty()) {
            d->audioFilePath.clear();
            Q_EMIT piece->audioFileChanged();
        }
    }

    void SynthesisAudioController::discard(SynthesisPiece *piece) {
        destroyBinding(m_bindings.take(piece));
    }

    void SynthesisAudioController::detachSeries(QPointer<Audio::TrackAudioContext> &trackContext, talcs::FutureAudioSourceClipSeries *&series) {
        if (!series) {
            return;
        }
        const auto pieces = m_bindings.keys();
        for (auto piece : pieces) {
            auto binding = m_bindings.value(piece);
            if (binding && binding->series == series) {
                remove(piece);
            }
        }
        if (trackContext) {
            trackContext->trackMixer()->removeSource(series);
        }
        delete series;
        series = nullptr;
        trackContext = nullptr;
    }

    void SynthesisAudioController::rebindClip(dspx::SingingClip *clip, SynthesisPiece *piece) {
        const auto binding = m_bindings.value(piece);
        auto mixer = binding ? binding->mixer : nullptr;
        if (!clip || !mixer) {
            return;
        }
        mixer->setGain(static_cast<float>(clip->gain()));
        mixer->setPan(static_cast<float>(clip->pan()));
        mixer->setSilentFlags(clip->mute() ? -1 : 0);
        connect(clip, &dspx::Clip::gainChanged, mixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(clip, &dspx::Clip::panChanged, mixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(clip, &dspx::Clip::muteChanged, mixer, [mixer](bool mute) {
            mixer->setSilentFlags(mute ? -1 : 0);
        });
    }

    bool SynthesisAudioController::hasBinding(SynthesisPiece *piece) const {
        const auto binding = m_bindings.value(piece);
        return binding && binding->futureSource;
    }

    bool SynthesisAudioController::isCanceled(SynthesisPiece *piece) const {
        const auto binding = m_bindings.value(piece);
        return binding && binding->futureSource && binding->futureSource->status() == talcs::FutureAudioSource::Cancelled;
    }

    bool SynthesisAudioController::isLoaded(SynthesisPiece *piece) const {
        const auto binding = m_bindings.value(piece);
        return binding && binding->futureSource && binding->futureSource->source();
    }

}
