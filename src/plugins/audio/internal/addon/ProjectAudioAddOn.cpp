#include "ProjectAudioAddOn.h"

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsDspx/DspxProjectContext.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodel/BusControl.h>
#include <dspxmodel/Master.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackControl.h>
#include <dspxmodel/TrackList.h>

#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/private/ProjectAudioContext_p.h>
#include <audio/TrackAudioContext.h>
#include <audio/private/TrackAudioContext_p.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Audio::Internal {

    ProjectAudioAddOn::ProjectAudioAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ProjectAudioAddOn::~ProjectAudioAddOn() {
        if (m_context) {
            GlobalAudioContext::preMixer()->removeSource(m_context->preMixer());
            const auto tracks = windowHandle()->cast<Core::ProjectWindowInterface>()->projectDocumentContext()->document()->model()->tracks()->items();
            for (auto track : tracks) {
                delete TrackAudioContext::of(track);
            }
        }
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
            return static_cast<qint64>(msec * sampleRate / 1000);
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

    void ProjectAudioAddOn::addTrack(int index, dspx::Track *track) {
        Q_ASSERT(TrackAudioContext::of(track) == nullptr);
        auto projectAudioContext = ProjectAudioContextPrivate::of(m_context)->projectContext.get();
        auto context = TrackAudioContextPrivate::create(windowHandle()->cast<Core::ProjectWindowInterface>(), track, projectAudioContext, index);
        syncTrackControl(track, context);
    }

    void ProjectAudioAddOn::removeTrack(int index, dspx::Track *track) {
        Q_UNUSED(index)
        auto context = TrackAudioContext::of(track);
        Q_ASSERT(context);
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
        auto masterModel = windowInterface->projectDocumentContext()->document()->model()->master();
        auto masterControlModel = masterModel->control();
        auto masterControlMixer = m_context->masterControlMixer();
        masterControlMixer->setRouteChannels(masterModel->multiChannelOutput());
        masterControlMixer->setGain(static_cast<float>(masterControlModel->gain()));
        masterControlMixer->setPan(static_cast<float>(masterControlModel->pan()));
        masterControlMixer->setSilentFlags(masterControlModel->mute() ? -1 : 0);
        connect(masterModel, &dspx::Master::multiChannelOutputChanged, masterControlMixer, &talcs::PositionableMixerAudioSource::setRouteChannels);
        connect(masterControlModel, &dspx::BusControl::gainChanged, masterControlMixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(masterControlModel, &dspx::BusControl::panChanged, masterControlMixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(masterControlModel, &dspx::BusControl::muteChanged, masterControlMixer, [masterControlMixer](bool mute) {
            masterControlMixer->setSilentFlags(mute ? -1 : 0);
        });
    }

    void ProjectAudioAddOn::syncTrackControl(dspx::Track *track, TrackAudioContext *context) {
        auto control = track->control();
        auto controlMixer = context->controlMixer();
        auto masterTrackMixer = m_context->masterTrackMixer();

        controlMixer->setGain(static_cast<float>(control->gain()));
        controlMixer->setPan(static_cast<float>(control->pan()));
        controlMixer->setSilentFlags(control->mute() ? -1 : 0);
        masterTrackMixer->setSourceSolo(controlMixer, control->solo());

        connect(control, &dspx::TrackControl::gainChanged, controlMixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(control, &dspx::TrackControl::panChanged, controlMixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(control, &dspx::TrackControl::muteChanged, controlMixer, [controlMixer](bool mute) {
            controlMixer->setSilentFlags(mute ? -1 : 0);
        });
        connect(control, &dspx::TrackControl::soloChanged, masterTrackMixer, [masterTrackMixer, controlMixer](bool solo) {
            masterTrackMixer->setSourceSolo(controlMixer, solo);
        });
    }

}
