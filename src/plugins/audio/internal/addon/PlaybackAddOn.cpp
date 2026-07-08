#include "PlaybackAddOn.h"

#include <QQmlComponent>

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
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());

        m_context = ProjectAudioContext::of(windowInterface);
        Q_ASSERT(m_context);
        m_projectTimeline = windowInterface->projectTimeline();
        Q_ASSERT(m_projectTimeline);
        m_documentModel = windowInterface->projectDocumentContext()->document()->model();
        Q_ASSERT(m_documentModel);

        auto transport = m_context->transport();
        connect(transport, &talcs::TransportAudioSource::positionAboutToChange, this, [this](qint64 positionSample) {
            m_transportPositionFlag = false;
            m_projectTimeline->setPosition(sampleToTick(positionSample));
            m_transportPositionFlag = true;
        });
        connect(transport, &talcs::TransportAudioSource::playbackStatusChanged, this, [this](talcs::TransportAudioSource::PlaybackStatus status) {
            if (status == talcs::TransportAudioSource::Paused &&
                m_context->status() == ProjectAudioContext::Stopped &&
                AudioPreference::playbackBehavior() == AudioPreference::PB_ReturnToStart) {
                m_projectTimeline->setPosition(m_projectTimeline->lastPosition());
            }
        });

        connect(m_context, &ProjectAudioContext::statusChanged, this, &PlaybackAddOn::handlePlaybackStatusChanged);
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
    }

    bool PlaybackAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    PlaybackAddOn *PlaybackAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<PlaybackAddOn>();
    }

    void PlaybackAddOn::play() {
        if (m_context) {
            // TODO check
            m_context->setStatus(ProjectAudioContext::Playing);
        }
    }

    void PlaybackAddOn::pause() {
        if (m_context) {
            m_context->setStatus(ProjectAudioContext::Paused);
        }
    }

    void PlaybackAddOn::stop() {
        if (m_context) {
            m_context->setStatus(ProjectAudioContext::Stopped);
        }
    }

    void PlaybackAddOn::togglePlayback() {
        if (m_context) {
            if (m_context->status() == ProjectAudioContext::Playing) {
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
        auto transport = m_context->transport();
        switch (status) {
            case ProjectAudioContext::Stopped:
                if (transport->playbackStatus() != talcs::TransportAudioSource::Paused) {
                    transport->pause();
                } else {
                    if (AudioPreference::playbackBehavior() == AudioPreference::PB_ReturnToStart) {
                        m_projectTimeline->setPosition(m_projectTimeline->lastPosition());
                    }
                }
                break;
            case ProjectAudioContext::Playing: {
                auto device = GlobalAudioContext::device();
                if (device && device->isOpen() && !device->isStarted()) {
                    device->start(GlobalAudioContext::playback());
                }
                if (m_lastStatus == ProjectAudioContext::Stopped) {
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
                transport->pause();
                break;
        }
        m_lastStatus = status;
    }

    void PlaybackAddOn::handlePlaybackPositionChanged(int positionTick) {
        if (m_transportPositionFlag) {
            m_context->transport()->setPosition(tickToSample(positionTick));
        }
    }

}

#include "moc_PlaybackAddOn.cpp"
