// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PlaybackAddOn.h"

#include <QPointer>
#include <QQmlComponent>
#include <QScopedValueRollback>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <TalcsCore/TransportAudioSource.h>
#include <TalcsDevice/AudioDevice.h>
#include <TalcsDevice/AudioSourcePlayback.h>

#include <dspxmodelORM/Model.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/internal/AudioPreference.h>
#include <audio/internal/EffectsAddOn.h>
#include <audio/internal/MidiRecordingAddOn.h>
#include <audio/private/ProjectAudioContext_p.h>

namespace Audio::Internal {

    PlaybackAddOn::PlaybackAddOn(QObject *parent)
        : WindowInterfaceAddOn(parent), m_lastStatus(ProjectAudioContext::Stopped) {
    }

    PlaybackAddOn::~PlaybackAddOn() = default;

    void PlaybackAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "PlaybackActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        if (!o) {
            qFatal() << component.errorString();
        }
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());

        m_context = ProjectAudioContext::of(windowInterface);
        Q_ASSERT(m_context);
        ProjectAudioContextPrivate::of(m_context)->playbackStatusChangeHandler = [playback = QPointer<PlaybackAddOn>(this)](ProjectAudioContext::PlaybackStatus status) {
            if (!playback)
                return false;
            auto recorder = MidiRecordingAddOn::of(playback->windowHandle()->cast<Core::ProjectWindowInterface>());
            if (recorder)
                return recorder->handlePlaybackStatusChange(status);
            return status == ProjectAudioContext::Recording || status == ProjectAudioContext::RecordingPaused;
        };
        m_projectTimeline = windowInterface->projectTimeline();
        Q_ASSERT(m_projectTimeline);
        m_documentModel = windowInterface->projectDocumentContext()->document()->model();
        Q_ASSERT(m_documentModel);

        auto transport = m_context->transport();
        connect(transport, &talcs::TransportAudioSource::positionAboutToChange, this, [this] {
            if (auto recorder = MidiRecordingAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()); recorder && recorder->isBusy())
                return;
            if (!m_context->isRunning())
                return;
            QScopedValueRollback positionFlag(m_transportPositionFlag, false);
            m_projectTimeline->setPosition(sampleToTick(m_context->transport()->position()));
        }, Qt::QueuedConnection);
        connect(transport, &talcs::TransportAudioSource::playbackStatusChanged, this, [this](talcs::TransportAudioSource::PlaybackStatus status) {
            if (auto recorder = MidiRecordingAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()); recorder && recorder->isBusy())
                return;
            if (status == talcs::TransportAudioSource::Paused &&
                m_context->transport()->playbackStatus() == talcs::TransportAudioSource::Paused &&
                m_context->status() == ProjectAudioContext::Stopped &&
                AudioPreference::playbackBehavior() == AudioPreference::PB_ReturnToStart) {
                m_projectTimeline->setPosition(m_projectTimeline->lastPosition());
            }
        }, Qt::QueuedConnection);

        connect(m_context, &ProjectAudioContext::statusChanged, this, &PlaybackAddOn::handlePlaybackStatusChanged);
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::metronomeEnabledChanged,
                this, &PlaybackAddOn::metronomeEnabledChanged);
        connect(m_projectTimeline, &Core::ProjectTimeline::positionChanged, this, &PlaybackAddOn::handlePlaybackPositionChanged);
        connect(m_projectTimeline->musicTimeline(), &SVS::MusicTimeline::changed, this, [this] {
            syncLoopingRange();
            handlePlaybackPositionChanged(m_projectTimeline->position());
        });
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::sampleRateChanged, this, [this] {
            syncLoopingRange();
            handlePlaybackPositionChanged(m_projectTimeline->position());
        });
        connect(m_documentModel, &dspx::Model::loopEnabledChanged, this, &PlaybackAddOn::syncLoopingRange);
        connect(m_documentModel, &dspx::Model::loopStartChanged, this, &PlaybackAddOn::syncLoopingRange);
        connect(m_documentModel, &dspx::Model::loopLengthChanged, this, &PlaybackAddOn::syncLoopingRange);

        syncLoopingRange();
        handlePlaybackPositionChanged(m_projectTimeline->position());
    }

    void PlaybackAddOn::extensionsInitialized() {
        if (auto recorder = MidiRecordingAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()))
            connect(recorder, &MidiRecordingAddOn::stateChanged, this, &PlaybackAddOn::syncLoopingRange);
    }

    bool PlaybackAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    PlaybackAddOn *PlaybackAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<PlaybackAddOn>();
    }

    void PlaybackAddOn::play() {
        cancelPendingPlayback();
        if (auto recorder = MidiRecordingAddOn::activeRecording()) {
            recorder->requestPlayback(this, m_playRequestSerial);
            return;
        }
        if (m_context && m_context->status() != ProjectAudioContext::Playing) {
            handlePlaybackPositionChanged(m_projectTimeline->position());
            m_context->setStatus(ProjectAudioContext::Playing);
        }
    }

    void PlaybackAddOn::pause() {
        cancelPendingPlayback();
        if (auto recorder = MidiRecordingAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()); recorder && recorder->isBusy()) {
            recorder->cancelPendingPlayback();
            if (recorder->isRecording())
                recorder->pauseRecording();
            return;
        }
        if (m_context) {
            m_context->setStatus(ProjectAudioContext::Paused);
        }
    }

    void PlaybackAddOn::stop() {
        cancelPendingPlayback();
        if (auto recorder = MidiRecordingAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()); recorder && recorder->isBusy()) {
            recorder->cancelPendingPlayback();
            if (!recorder->isRecording() && recorder->m_state != MidiRecordingAddOn::AwaitingQuantization)
                return;
            recorder->m_stopRequested = true;
            recorder->stopRecording();
            return;
        }
        if (m_context) {
            m_context->setStatus(ProjectAudioContext::Stopped);
        }
    }

    void PlaybackAddOn::togglePlayback() {
        if (m_context) {
            if (m_context->isRunning()) {
                if (AudioPreference::playbackTogglingAction() == AudioPreference::PTA_PlayPause) {
                    pause();
                } else {
                    stop();
                }
            } else {
                play();
            }
        }
    }

    void PlaybackAddOn::cancelPendingPlayback() {
        ++m_playRequestSerial;
        if (auto recorder = MidiRecordingAddOn::activeRecording())
            recorder->cancelPendingPlayback(this);
    }

    void PlaybackAddOn::playAfterRecording(quint64 serial) {
        if (serial != m_playRequestSerial || MidiRecordingAddOn::activeRecording() || windowHandle()->isEffectivelyClosed())
            return;
        windowHandle()->window()->requestActivate();
        play();
    }

    void PlaybackAddOn::finishRecordingPlayback(bool stopRequested) {
        syncLoopingRange();
        if (stopRequested && AudioPreference::playbackBehavior() == AudioPreference::PB_ReturnToStart)
            m_projectTimeline->setPosition(m_projectTimeline->lastPosition());
        if (auto effectsAddOn = EffectsAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()))
            effectsAddOn->refreshAllEffects();
        m_context->transport()->setPosition(tickToSample(m_projectTimeline->position()));
    }

    static QObject *createActionObject(QAK::QuickActionContext *actionContext, const QString &id) {
        auto component = actionContext->action(id);
        Q_ASSERT(component);
        auto object = component->create(component->creationContext());
        Q_ASSERT(object);
        actionContext->attachActionInfo(id, object);
        return object;
    }

    QObject *PlaybackAddOn::getPlayAction() const {
        return createActionObject(windowHandle()->cast<Core::ProjectWindowInterface>()->actionContext(), "org.diffscope.audio.playback.play");
    }

    QObject *PlaybackAddOn::getPauseAction() const {
        return createActionObject(windowHandle()->cast<Core::ProjectWindowInterface>()->actionContext(), "org.diffscope.audio.playback.pause");
    }

    bool PlaybackAddOn::metronomeEnabled() const {
        return GlobalAudioContext::metronomeEnabled();
    }

    void PlaybackAddOn::setMetronomeEnabled(bool enabled) {
        GlobalAudioContext::setMetronomeEnabled(enabled);
    }

    qint64 PlaybackAddOn::tickToSample(int tick) const {
        auto sampleRate = GlobalAudioContext::sampleRate();
        if (qFuzzyIsNull(sampleRate)) {
            return 0;
        }
        auto msec = m_projectTimeline->musicTimeline()->create(0, 0, tick).millisecond();
        return static_cast<qint64>(msec * sampleRate / 1000);
    }

    int PlaybackAddOn::sampleToTick(qint64 sample) const {
        auto sampleRate = GlobalAudioContext::sampleRate();
        if (qFuzzyIsNull(sampleRate)) {
            return 0;
        }
        auto msec = static_cast<double>(sample) * 1000 / sampleRate;
        return m_projectTimeline->musicTimeline()->create(msec).totalTick();
    }

    void PlaybackAddOn::syncLoopingRange() {
        if (auto recorder = MidiRecordingAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()); recorder && recorder->isBusy()) {
            m_context->transport()->setLoopingRange(-1, -1);
            return;
        }
        if (!m_documentModel->loopEnabled()) {
            m_context->transport()->setLoopingRange(-1, -1);
            return;
        }

        const auto loopStart = m_documentModel->loopStart();
        const auto loopLength = m_documentModel->loopLength();
        if (loopLength <= 0) {
            m_context->transport()->setLoopingRange(-1, -1);
            return;
        }

        m_context->transport()->setLoopingRange(
            tickToSample(loopStart),
            tickToSample(loopStart + loopLength)
        );
    }

    void PlaybackAddOn::handlePlaybackStatusChanged(ProjectAudioContext::PlaybackStatus status) {
        auto recorder = MidiRecordingAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>());
        auto transport = m_context->transport();
        switch (status) {
            case ProjectAudioContext::Stopped:
                if (transport->playbackStatus() != talcs::TransportAudioSource::Paused) {
                    transport->pause();
                } else {
                    if ((!recorder || !recorder->isBusy()) && AudioPreference::playbackBehavior() == AudioPreference::PB_ReturnToStart) {
                        m_projectTimeline->setPosition(m_projectTimeline->lastPosition());
                    }
                }
                break;
            case ProjectAudioContext::Playing:
            case ProjectAudioContext::Recording: {
                auto device = GlobalAudioContext::device();
                if (device && device->isOpen() && !device->isStarted()) {
                    device->start(GlobalAudioContext::playback());
                }
                if (status == ProjectAudioContext::Playing && m_lastStatus == ProjectAudioContext::Stopped && (!recorder || !recorder->isBusy())) {
                    if (AudioPreference::playbackBehavior() == AudioPreference::PB_KeepAtCurrentButPlayFromStart) {
                        m_projectTimeline->setPosition(m_projectTimeline->lastPosition());
                    } else {
                        m_projectTimeline->setLastPosition(m_projectTimeline->position());
                    }
                }
                transport->play();
                break;
            }
            case ProjectAudioContext::Paused:
            case ProjectAudioContext::RecordingPaused:
                transport->pause();
                break;
        }
        m_lastStatus = status;
    }

    void PlaybackAddOn::handlePlaybackPositionChanged(int positionTick) {
        if (auto recorder = MidiRecordingAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()); recorder && recorder->isBusy())
            return;
        if (m_transportPositionFlag) {
            if (auto effectsAddOn = EffectsAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>())) {
                effectsAddOn->refreshAllEffects();
            }
            m_context->transport()->setPosition(tickToSample(positionTick));
        }
    }

}

#include "moc_PlaybackAddOn.cpp"
