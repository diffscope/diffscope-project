#include "ProjectAudioAddOn.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsDspx/DspxAudioClipContext.h>
#include <TalcsDspx/DspxProjectContext.h>
#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
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
        return !expected.isEmpty() && HashHelper::sha512(filePath).compare(expected, Qt::CaseInsensitive) == 0;
    }

    ProjectAudioAddOn::ProjectAudioAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ProjectAudioAddOn::~ProjectAudioAddOn() {
        if (m_context) {
            GlobalAudioContext::preMixer()->removeSource(m_context->preMixer());
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
            auto sampleRate = GlobalAudioContext::sampleRate();
            if (qFuzzyIsNull(sampleRate))
                return 0;
            return static_cast<qint64>(std::round(msec * sampleRate / 1000));
        });

        syncMasterControl();

        auto trackList = windowInterface->projectDocumentContext()->document()->model()->tracks();
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
                if (!sha512Matches(filePath, path.sha512)) {
                    status = AudioClipAudioContext::FileContentChanged;
                }
            } else {
                auto projectDocumentContext = windowHandle()->cast<Core::ProjectWindowInterface>()->projectDocumentContext();
                auto fileLocker = projectDocumentContext->fileLocker();
                const auto projectFilePath = fileLocker ? fileLocker->path() : QString();
                if (!projectFilePath.isEmpty()) {
                    const auto projectDir = QFileInfo(projectFilePath).absoluteDir();
                    const auto relativeFilePath = normalizedAbsolutePath(projectDir.filePath(QDir(path.relativeDir).filePath(path.fileName)));
                    if (QFileInfo(relativeFilePath).isFile() && sha512Matches(relativeFilePath, path.sha512)) {
                        filePath = relativeFilePath;
                    } else {
                        const auto siblingFilePath = normalizedAbsolutePath(projectDir.filePath(path.fileName));
                        if (QFileInfo(siblingFilePath).isFile() && sha512Matches(siblingFilePath, path.sha512)) {
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
            if (!clip->path().sha512.isEmpty()) {
                // Empty file digest suggests that the project file might be created by another editor. We do not explicitly notify user in this case.
                windowInterface->sendNotification(SVS::SVSCraft::Warning, tr("Audio file content changed"), tr("The file in audio clip \"%1\" has been changed:\n%2").arg(clip->name(), path));
            }
        }
    }

}
