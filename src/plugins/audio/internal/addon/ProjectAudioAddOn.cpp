#include "ProjectAudioAddOn.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

#include <TalcsCore/MetronomeAudioSource.h>
#include <TalcsCore/MixerAudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsCore/TransportAudioSource.h>
#include <TalcsDspx/DspxAudioClipContext.h>
#include <TalcsDspx/DspxProjectContext.h>
#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/MusicTimeSignature.h>

#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/TimeSignature.h>
#include <dspxmodelORM/TimeSignatureSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <audio/AudioClipAudioContext.h>
#include <audio/private/AudioClipAudioContext_p.h>
#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/private/ProjectAudioContext_p.h>
#include <audio/TrackAudioContext.h>
#include <audio/private/TrackAudioContext_p.h>
#include <audio/internal/HashHelper.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <CoreApi/filelocker.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcProjectAudioAddOn, "diffscope.audio.projectaudioaddon")

    static QString normalizedAbsolutePath(const QString &filePath) {
        return QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
    }

    static bool sha512Matches(const QString &filePath, const QString &expected) {
        return !expected.isEmpty() && HashHelper::digest(filePath).compare(expected, Qt::CaseInsensitive) == 0;
    }

    ProjectAudioAddOn::ProjectAudioAddOn(QObject *parent)
        : WindowInterfaceAddOn(parent), m_musicTimeline(std::make_unique<SVS::MusicTimeline>()) {
    }

    ProjectAudioAddOn::~ProjectAudioAddOn() {
        if (m_context) {
            GlobalAudioContext::preMixer()->removeSource(m_context->preMixer());
            if (m_metronomeAudioSource) {
                m_metronomeAudioSource->setDetector(nullptr);
            }
            const auto tracks = windowHandle()->cast<Core::ProjectWindowInterface>()->projectDocumentContext()->document()->model()->tracks()->items();
            for (auto track : tracks) {
                for (auto clip : track->clips()->asRange()) {
                    removeClip(clip);
                }
                delete TrackAudioContext::of(track);
            }
        }
        qDeleteAll(m_audioClipCache);
    }

    void ProjectAudioAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);
        Q_ASSERT(ProjectAudioContext::of(windowInterface) == nullptr);
        m_context = ProjectAudioContextPrivate::create(windowInterface);
        windowInterface->addObject(m_context);
        GlobalAudioContext::preMixer()->addSource(m_context->preMixer());

        auto dspxProjectContext = ProjectAudioContextPrivate::of(m_context)->projectContext.get();
        dspxProjectContext->setFormatManager(GlobalAudioContext::formatManager());
        dspxProjectContext->setTimeConverter([=](int tick) -> qint64 {
            auto timeline = windowInterface->projectTimeline()->musicTimeline();
            auto msec = timeline->create(0, 0, tick).millisecond();
            const auto activeSampleRate = dspxProjectContext->preMixer()->sampleRate();
            const auto sampleRate = qFuzzyIsNull(activeSampleRate)
                ? GlobalAudioContext::sampleRate()
                : activeSampleRate;
            if (qFuzzyIsNull(sampleRate))
                return 0;
            return static_cast<qint64>(std::round(msec * sampleRate / 1000));
        });

        auto model = windowInterface->projectDocumentContext()->document()->model();
        auto tempoSequence = model->tempos();
        auto timeSignatureSequence = model->timeSignatures();
        for (auto tempo : tempoSequence->asRange()) {
            handleTempoInsertedOrUpdated(tempo);
        }
        for (auto timeSignature : timeSignatureSequence->asRange()) {
            handleTimeSignatureInsertedOrUpdated(timeSignature);
        }
        connect(tempoSequence, &dspx::TempoSequence::itemInserted,
                this, &ProjectAudioAddOn::handleTempoInsertedOrUpdated);
        connect(tempoSequence, &dspx::TempoSequence::itemRemoved,
                this, &ProjectAudioAddOn::handleTempoRemoved);
        connect(timeSignatureSequence, &dspx::TimeSignatureSequence::itemInserted,
                this, &ProjectAudioAddOn::handleTimeSignatureInsertedOrUpdated);
        connect(timeSignatureSequence, &dspx::TimeSignatureSequence::itemRemoved,
                this, &ProjectAudioAddOn::handleTimeSignatureRemoved);

        auto transport = m_context->transport();
        {
            QMutexLocker locker(&m_transportPositionMutex);
            m_transportPosition = transport->position();
        }
        connect(transport, &talcs::TransportAudioSource::positionAboutToChange, this,
                [this](qint64 position) {
                    QMutexLocker locker(&m_transportPositionMutex);
                    m_transportPosition = position;
                }, Qt::DirectConnection);

        auto metronomeControlMixer = m_context->metronomeControlMixer();
        metronomeControlMixer->setSilentFlags(GlobalAudioContext::metronomeEnabled() ? 0 : -1);
        metronomeControlMixer->setGain(static_cast<float>(GlobalAudioContext::metronomeGain()));
        metronomeControlMixer->setPan(static_cast<float>(GlobalAudioContext::metronomePan()));
        m_metronomeAudioSource = new talcs::MetronomeAudioSource;
        m_metronomeAudioSource->setMajorBeatSource(talcs::MetronomeAudioSource::builtInMajorBeatSource(), true);
        m_metronomeAudioSource->setMinorBeatSource(talcs::MetronomeAudioSource::builtInMinorBeatSource(), true);
        m_metronomeAudioSource->setDetector(this);
        metronomeControlMixer->addSource(m_metronomeAudioSource, true);
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::metronomeEnabledChanged, this,
                [metronomeControlMixer](bool enabled) {
                    metronomeControlMixer->setSilentFlags(enabled ? 0 : -1);
                });
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::metronomeGainChanged, this,
                [metronomeControlMixer](double gain) {
                    metronomeControlMixer->setGain(static_cast<float>(gain));
                });
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::metronomePanChanged, this,
                [metronomeControlMixer](double pan) {
                    metronomeControlMixer->setPan(static_cast<float>(pan));
                });

        syncMasterControl();

        auto trackList = model->tracks();
        const auto tracks = trackList->items();
        for (int i = 0; i < tracks.size(); ++i) {
            addTrack(i, tracks.at(i));
        }
        connect(trackList, &dspx::TrackList::itemInserted, this, &ProjectAudioAddOn::addTrack);
        connect(trackList, &dspx::TrackList::itemRemoved, this, &ProjectAudioAddOn::removeTrack);
        connect(trackList, &dspx::TrackList::rotated, this, &ProjectAudioAddOn::rotateTrack);
    }

    void ProjectAudioAddOn::extensionsInitialized() {
    }

    bool ProjectAudioAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    ProjectAudioAddOn *ProjectAudioAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<ProjectAudioAddOn>();
    }

    void ProjectAudioAddOn::addAudioClipCache(dspx::AudioClip *clip, talcs::AbstractAudioFormatIO *io) {
        delete m_audioClipCache.take(clip);
        if (io) {
            m_audioClipCache.insert(clip, io);
        }
    }

    talcs::AbstractAudioFormatIO *ProjectAudioAddOn::takeAudioClipCache(dspx::AudioClip *clip) {
        return m_audioClipCache.take(clip);
    }

    void ProjectAudioAddOn::handleTempoInsertedOrUpdated(dspx::Tempo *tempo) {
        const bool isNewTempo = !m_tempoPosMap.contains(tempo);
        if (isNewTempo) {
            connect(tempo, &dspx::Tempo::valueChanged, this, [this, tempo] {
                handleTempoInsertedOrUpdated(tempo);
            });
            connect(tempo, &dspx::Tempo::positionChanged, this, [this, tempo] {
                handleTempoInsertedOrUpdated(tempo);
            });
        }

        QMutexLocker locker(&m_musicTimelineMutex);
        if (!isNewTempo) {
            const int previousPosition = m_tempoPosMap.value(tempo);
            auto &tempoSet = m_tempoMap[previousPosition];
            tempoSet.remove(tempo);
            if (tempoSet.isEmpty()) {
                if (previousPosition != 0) {
                    m_musicTimeline->removeTempo(previousPosition);
                }
            } else {
                m_musicTimeline->setTempo(previousPosition, (*tempoSet.begin())->value());
            }
        }

        m_tempoPosMap.insert(tempo, tempo->position());
        auto &tempoSet = m_tempoMap[tempo->position()];
        tempoSet.insert(tempo);
        m_musicTimeline->setTempo(tempo->position(), (*tempoSet.begin())->value());
    }

    void ProjectAudioAddOn::handleTempoRemoved(dspx::Tempo *tempo) {
        if (!m_tempoPosMap.contains(tempo)) {
            return;
        }

        {
            QMutexLocker locker(&m_musicTimelineMutex);
            const int position = m_tempoPosMap.value(tempo);
            auto &tempoSet = m_tempoMap[position];
            tempoSet.remove(tempo);
            if (tempoSet.isEmpty()) {
                if (position != 0) {
                    m_musicTimeline->removeTempo(position);
                }
            } else {
                m_musicTimeline->setTempo(position, (*tempoSet.begin())->value());
            }
            m_tempoPosMap.remove(tempo);
        }
        disconnect(tempo, nullptr, this, nullptr);
    }

    void ProjectAudioAddOn::handleTimeSignatureInsertedOrUpdated(dspx::TimeSignature *timeSignature) {
        const bool isNewTimeSignature = !m_timeSignatureMeasureMap.contains(timeSignature);
        if (isNewTimeSignature) {
            connect(timeSignature, &dspx::TimeSignature::numeratorChanged, this, [this, timeSignature] {
                handleTimeSignatureInsertedOrUpdated(timeSignature);
            });
            connect(timeSignature, &dspx::TimeSignature::denominatorChanged, this, [this, timeSignature] {
                handleTimeSignatureInsertedOrUpdated(timeSignature);
            });
            connect(timeSignature, &dspx::TimeSignature::indexChanged, this, [this, timeSignature] {
                handleTimeSignatureInsertedOrUpdated(timeSignature);
            });
        }

        QMutexLocker locker(&m_musicTimelineMutex);
        if (!isNewTimeSignature) {
            const int previousMeasure = m_timeSignatureMeasureMap.value(timeSignature);
            auto &timeSignatureSet = m_timeSignatureMap[previousMeasure];
            timeSignatureSet.remove(timeSignature);
            if (timeSignatureSet.isEmpty()) {
                if (previousMeasure != 0) {
                    m_musicTimeline->removeTimeSignature(previousMeasure);
                }
            } else {
                auto item = *timeSignatureSet.begin();
                m_musicTimeline->setTimeSignature(previousMeasure, {item->numerator(), item->denominator()});
            }
        }

        m_timeSignatureMeasureMap.insert(timeSignature, timeSignature->index());
        auto &timeSignatureSet = m_timeSignatureMap[timeSignature->index()];
        timeSignatureSet.insert(timeSignature);
        auto item = *timeSignatureSet.begin();
        m_musicTimeline->setTimeSignature(timeSignature->index(), {item->numerator(), item->denominator()});
    }

    void ProjectAudioAddOn::handleTimeSignatureRemoved(dspx::TimeSignature *timeSignature) {
        if (!m_timeSignatureMeasureMap.contains(timeSignature)) {
            return;
        }

        {
            QMutexLocker locker(&m_musicTimelineMutex);
            const int measure = m_timeSignatureMeasureMap.value(timeSignature);
            auto &timeSignatureSet = m_timeSignatureMap[measure];
            timeSignatureSet.remove(timeSignature);
            if (timeSignatureSet.isEmpty()) {
                if (measure != 0) {
                    m_musicTimeline->removeTimeSignature(measure);
                }
            } else {
                auto item = *timeSignatureSet.begin();
                m_musicTimeline->setTimeSignature(measure, {item->numerator(), item->denominator()});
            }
            m_timeSignatureMeasureMap.remove(timeSignature);
        }
        disconnect(timeSignature, nullptr, this, nullptr);
    }

    void ProjectAudioAddOn::detectInterval(qint64 intervalLength) {
        m_metronomeMessages.clear();
        m_nextMetronomeMessageIndex = 0;

        if (!GlobalAudioContext::metronomeEnabled() || intervalLength <= 0) {
            return;
        }

        auto transport = m_context->transport();
        if (transport->playbackStatus() == talcs::TransportAudioSource::Paused ||
            transport->bufferingCounter() != 0) {
            return;
        }

        const double sampleRate = m_metronomeAudioSource->sampleRate();
        if (qFuzzyIsNull(sampleRate)) {
            return;
        }

        qint64 intervalStart;
        {
            QMutexLocker locker(&m_transportPositionMutex);
            intervalStart = m_transportPosition;
        }
        if (intervalStart < 0) {
            return;
        }

        const double intervalStartMsec = static_cast<double>(intervalStart) * 1000.0 / sampleRate;
        const double intervalEndMsec = static_cast<double>(intervalStart + intervalLength) * 1000.0 / sampleRate;
        QVector<talcs::MetronomeAudioSourceDetectorMessage> messages;

        {
            QMutexLocker locker(&m_musicTimelineMutex);
            auto beat = m_musicTimeline->create(intervalStartMsec).ceilBeat();
            while (beat.isValid()) {
                const double beatMsec = beat.millisecond();
                if (beatMsec >= intervalEndMsec) {
                    break;
                }

                const qint64 relativePosition = qRound64(beatMsec * sampleRate / 1000.0) - intervalStart;
                if (relativePosition >= intervalLength) {
                    break;
                }
                if (relativePosition >= 0) {
                    messages.append({relativePosition, beat.beat() == 0});
                }

                auto nextBeat = beat.nextBeat();
                if (!nextBeat.isValid() || nextBeat.totalTick() <= beat.totalTick()) {
                    break;
                }
                beat = std::move(nextBeat);
            }
        }

        m_metronomeMessages = std::move(messages);
    }

    talcs::MetronomeAudioSourceDetectorMessage ProjectAudioAddOn::nextMessage() {
        if (!GlobalAudioContext::metronomeEnabled() ||
            m_nextMetronomeMessageIndex >= m_metronomeMessages.size()) {
            return {-1, false};
        }
        return m_metronomeMessages.at(m_nextMetronomeMessageIndex++);
    }

    void ProjectAudioAddOn::addTrack(int index, dspx::Track *track) {
        Q_ASSERT(TrackAudioContext::of(track) == nullptr);
        auto projectAudioContext = ProjectAudioContextPrivate::of(m_context)->projectContext.get();
        auto context = TrackAudioContextPrivate::create(windowHandle()->cast<Core::ProjectWindowInterface>(), track, projectAudioContext, index);
        syncTrackControl(track, context);
        syncTrackClips(track, context);
    }

    void ProjectAudioAddOn::removeTrack(int index, dspx::Track *track) {
        Q_UNUSED(index)
        auto context = TrackAudioContext::of(track);
        Q_ASSERT(context);
        for (auto clip : track->clips()->asRange()) {
            removeClip(clip);
        }
        delete context;
    }

    void ProjectAudioAddOn::rotateTrack(int leftIndex, int middleIndex, int rightIndex) {
        const auto count = middleIndex - leftIndex;
        if (count <= 0 || middleIndex == rightIndex) {
            return;
        }
        ProjectAudioContextPrivate::of(m_context)->projectContext->moveTrack(leftIndex, count, rightIndex);
    }

    void ProjectAudioAddOn::syncMasterControl() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto masterModel = windowInterface->projectDocumentContext()->document()->model();
        auto masterControlMixer = m_context->masterControlMixer();
        masterControlMixer->setRouteChannels(masterModel->multiChannelOutput());
        masterControlMixer->setGain(static_cast<float>(masterModel->gain()));
        masterControlMixer->setPan(static_cast<float>(masterModel->pan()));
        masterControlMixer->setSilentFlags(masterModel->mute() ? -1 : 0);
        connect(masterModel, &dspx::Model::multiChannelOutputChanged, masterControlMixer, &talcs::PositionableMixerAudioSource::setRouteChannels);
        connect(masterModel, &dspx::Model::gainChanged, masterControlMixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(masterModel, &dspx::Model::panChanged, masterControlMixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(masterModel, &dspx::Model::muteChanged, masterControlMixer, [masterControlMixer](bool mute) {
            masterControlMixer->setSilentFlags(mute ? -1 : 0);
        });
    }

    void ProjectAudioAddOn::syncTrackControl(dspx::Track *track, TrackAudioContext *context) {
        auto controlMixer = context->controlMixer();
        auto masterTrackMixer = m_context->masterTrackMixer();

        controlMixer->setGain(static_cast<float>(track->gain()));
        controlMixer->setPan(static_cast<float>(track->pan()));
        controlMixer->setSilentFlags(track->mute() ? -1 : 0);
        masterTrackMixer->setSourceSolo(controlMixer, track->solo());

        connect(track, &dspx::Track::gainChanged, controlMixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(track, &dspx::Track::panChanged, controlMixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(track, &dspx::Track::muteChanged, controlMixer, [controlMixer](bool mute) {
            controlMixer->setSilentFlags(mute ? -1 : 0);
        });
        connect(track, &dspx::Track::soloChanged, masterTrackMixer, [masterTrackMixer, controlMixer](bool solo) {
            masterTrackMixer->setSourceSolo(controlMixer, solo);
        });
    }

    void ProjectAudioAddOn::syncTrackClips(dspx::Track *track, TrackAudioContext *context) {
        Q_UNUSED(context)
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

    void ProjectAudioAddOn::addClip(dspx::Clip *clip) {
        if (!clip || clip->type() != dspx::Clip::Audio) {
            return;
        }
        auto audioClip = static_cast<dspx::AudioClip *>(clip);
        Q_ASSERT(AudioClipAudioContext::of(audioClip) == nullptr);
        auto trackContext = TrackAudioContext::of(clip->clipSequence()->track());
        Q_ASSERT(trackContext);
        auto context = AudioClipAudioContextPrivate::create(windowHandle()->cast<Core::ProjectWindowInterface>(), audioClip, TrackAudioContextPrivate::of(trackContext)->trackContext);
        syncAudioClip(audioClip, context);
    }

    void ProjectAudioAddOn::removeClip(dspx::Clip *clip) {
        if (!clip || clip->type() != dspx::Clip::Audio) {
            return;
        }
        auto context = AudioClipAudioContext::of(static_cast<dspx::AudioClip *>(clip));
        if (context) {
            delete context;
        }
    }

    void ProjectAudioAddOn::syncAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context) {
        auto controlMixer = context->controlMixer();

        controlMixer->setGain(static_cast<float>(clip->gain()));
        controlMixer->setPan(static_cast<float>(clip->pan()));
        controlMixer->setSilentFlags(clip->mute() ? -1 : 0);

        connect(clip, &dspx::Clip::gainChanged, controlMixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(clip, &dspx::Clip::panChanged, controlMixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(clip, &dspx::Clip::muteChanged, controlMixer, [controlMixer](bool mute) {
            controlMixer->setSilentFlags(mute ? -1 : 0);
        });

        auto clipContext = AudioClipAudioContextPrivate::of(context)->clipContext;
        clipContext->setStart(clip->start());
        clipContext->setClipStart(clip->clipStart());
        clipContext->setClipLen(clip->clipLength());

        connect(clip, &dspx::Clip::startChanged, clipContext, &talcs::DspxAudioClipContext::setStart);
        connect(clip, &dspx::Clip::clipStartChanged, clipContext, &talcs::DspxAudioClipContext::setClipStart);
        connect(clip, &dspx::Clip::clipLengthChanged, clipContext, &talcs::DspxAudioClipContext::setClipLen);
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::sampleRateChanged, clipContext, [clipContext] {
            clipContext->updatePosition();
        });
        connect(windowHandle()->cast<Core::ProjectWindowInterface>()->projectTimeline()->musicTimeline(), &SVS::MusicTimeline::tempiChanged, clipContext, [clipContext] {
            clipContext->updatePosition();
        });

        loadAudioClip(clip, context);
        clipContext->updatePosition();

        connect(clip, &dspx::AudioClip::pathChanged, context, [this, clip, context] {
            reloadAudioClip(clip, context);
        });
    }

    void ProjectAudioAddOn::loadAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context) {
        auto contextPrivate = AudioClipAudioContextPrivate::of(context);
        auto io = takeAudioClipCache(clip);
        if (!io) {
            const auto path = clip->path();
            qCInfo(lcProjectAudioAddOn) << "Loading audio clip:" << path.absoluteDir << path.relativeDir << path.fileName;
            const auto absoluteFilePath = normalizedAbsolutePath(QDir(path.absoluteDir).filePath(path.fileName));
            QString filePath;
            auto status = AudioClipAudioContext::Ready;

            if (QFileInfo(absoluteFilePath).isFile()) {
                filePath = absoluteFilePath;
                if (!sha512Matches(filePath, path.digest)) {
                    status = AudioClipAudioContext::FileContentChanged;
                }
            } else {
                auto projectDocumentContext = windowHandle()->cast<Core::ProjectWindowInterface>()->projectDocumentContext();
                auto fileLocker = projectDocumentContext->fileLocker();
                const auto projectFilePath = fileLocker ? fileLocker->path() : QString();
                if (!projectFilePath.isEmpty()) {
                    const auto projectDir = QFileInfo(projectFilePath).absoluteDir();
                    const auto relativeFilePath = normalizedAbsolutePath(projectDir.filePath(QDir(path.relativeDir).filePath(path.fileName)));
                    if (QFileInfo(relativeFilePath).isFile() && sha512Matches(relativeFilePath, path.digest)) {
                        filePath = relativeFilePath;
                    } else {
                        const auto siblingFilePath = normalizedAbsolutePath(projectDir.filePath(path.fileName));
                        if (QFileInfo(siblingFilePath).isFile() && sha512Matches(siblingFilePath, path.digest)) {
                            filePath = siblingFilePath;
                        }
                    }
                    if (!filePath.isEmpty()) {
                        status = AudioClipAudioContext::FileMoved;
                    }
                }
            }

            if (filePath.isEmpty()) {
                contextPrivate->setRealAudioPath({});
                contextPrivate->setStatus(AudioClipAudioContext::FileNotFound);
                notifyAudioClipStatus(clip, context);
                return;
            }

            contextPrivate->setRealAudioPath(filePath);
            io = GlobalAudioContext::formatManager()->getFormatLoad(filePath, path.userData, path.formatEntryClassName);
            if (!io) {
                contextPrivate->setStatus(AudioClipAudioContext::FileLoadFailed);
                notifyAudioClipStatus(clip, context);
                return;
            }
            contextPrivate->setStatus(status);
        } else {
            const auto path = clip->path();
            contextPrivate->setRealAudioPath(normalizedAbsolutePath(QDir(path.absoluteDir).filePath(path.fileName)));
            contextPrivate->setStatus(AudioClipAudioContext::Ready);
        }

        contextPrivate->clipContext->loadAudio(io);
        if (context->status() != AudioClipAudioContext::Ready) {
            notifyAudioClipStatus(clip, context);
        }
    }

    void ProjectAudioAddOn::reloadAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context) {
        auto clipContext = AudioClipAudioContextPrivate::of(context)->clipContext;
        if (context->contentSource()) {
            delete clipContext->takeAudio();
        }
        loadAudioClip(clip, context);
        clipContext->updatePosition();
    }

    void ProjectAudioAddOn::notifyAudioClipStatus(dspx::AudioClip *clip, AudioClipAudioContext *context) {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto path = QDir::toNativeSeparators(QDir(clip->path().absoluteDir).filePath(clip->path().fileName));
        if (context->status() == AudioClipAudioContext::FileNotFound) {
            qCWarning(lcProjectAudioAddOn) << "Audio clip file not found:" << clip->name() << clip->path().absoluteDir << clip->path().relativeDir << clip->path().fileName;
            if (clip->path().absoluteDir.isEmpty() || clip->path().fileName.isEmpty()) {
                windowInterface->sendNotification(SVS::SVSCraft::Critical, tr("Audio file not specified"), tr("Audio clip \"%1\" has no file specified").arg(clip->name()));
            }
            windowInterface->sendNotification(SVS::SVSCraft::Critical, tr("Audio file not found"), tr("The file in audio clip \"%1\" is not found:\n%2").arg(clip->name(), path));
        } else if (context->status() == AudioClipAudioContext::FileLoadFailed) {
            qCWarning(lcProjectAudioAddOn) << "Audio clip file load failed:" << clip->name() << clip->path().absoluteDir << clip->path().relativeDir << clip->path().fileName;
            windowInterface->sendNotification(SVS::SVSCraft::Critical, tr("Audio file failed to load"), tr("Failed to load audio file in audio clip \"%1\":\n%2").arg(clip->name(), path));
        } else if (context->status() == AudioClipAudioContext::FileMoved) {
            qCWarning(lcProjectAudioAddOn) << "Audio clip file moved:" << clip->name() << clip->path().absoluteDir << clip->path().relativeDir << clip->path().fileName << "to" << context->realAudioPath();
            windowInterface->sendNotification(SVS::SVSCraft::Warning, tr("Audio file moved"), tr("The file in audio clip \"%1\" has been moved.\nFrom: %2\nTo: %3").arg(clip->name(), path, QDir::toNativeSeparators(context->realAudioPath())));
        } else if (context->status() == AudioClipAudioContext::FileContentChanged) {
            qCWarning(lcProjectAudioAddOn) << "Audio clip file content changed:" << clip->name() << clip->path().absoluteDir << clip->path().relativeDir << clip->path().fileName;
            if (!clip->path().digest.isEmpty()) {
                // Empty file digest suggests that the project file might be created by another editor. We do not explicitly notify user in this case.
                windowInterface->sendNotification(SVS::SVSCraft::Warning, tr("Audio file content changed"), tr("The file in audio clip \"%1\" has been changed:\n%2").arg(clip->name(), path));
            }
        }
    }

}
