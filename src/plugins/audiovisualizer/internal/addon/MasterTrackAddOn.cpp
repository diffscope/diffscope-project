#include "MasterTrackAddOn.h"

#include <ScopicFlowCore/ListViewModel.h>
#include <ScopicFlowCore/TrackViewModel.h>

#include <SVSCraftCore/DecibelLinearizer.h>

#include <audio/GlobalAudioContext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace AudioVisualizer::Internal {

    namespace {

        double toDecibels(double gain) {
            return SVS::DecibelLinearizer::gainToDecibels(gain);
        }

        double toGain(double decibels) {
            return SVS::DecibelLinearizer::decibelsToGain(decibels);
        }

    }

    MasterTrackAddOn::MasterTrackAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    MasterTrackAddOn::~MasterTrackAddOn() {
        if (!m_masterTrackListViewModel) {
            return;
        }

        const auto removeTrackViewModel = [this](sflow::TrackViewModel *viewModel) {
            if (!viewModel) {
                return;
            }
            const int index = m_masterTrackListViewModel->items().indexOf(viewModel);
            if (index >= 0) {
                m_masterTrackListViewModel->removeItem(index);
            }
        };
        removeTrackViewModel(m_metronomeTrackViewModel);
        removeTrackViewModel(m_deviceOutputTrackViewModel);
    }

    void MasterTrackAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);

        auto viewModelContext = VisualEditor::ProjectViewModelContext::of(windowInterface);
        Q_ASSERT(viewModelContext);
        if (!viewModelContext) {
            return;
        }

        m_masterTrackListViewModel = viewModelContext->masterTrackListViewModel();
        Q_ASSERT(m_masterTrackListViewModel);
        if (!m_masterTrackListViewModel) {
            return;
        }

        auto globalAudioContext = Audio::GlobalAudioContext::instance();

        m_metronomeTrackViewModel = new sflow::TrackViewModel(this);
        m_metronomeTrackViewModel->setName(tr("Metronome"));
        m_metronomeTrackViewModel->setGain(toDecibels(Audio::GlobalAudioContext::metronomeGain()));
        m_metronomeTrackViewModel->setPan(Audio::GlobalAudioContext::metronomePan());
        m_metronomeTrackViewModel->setMute(!Audio::GlobalAudioContext::metronomeEnabled());
        m_metronomeTrackViewModel->setMultiChannelOutput(false);

        connect(globalAudioContext, &Audio::GlobalAudioContext::metronomeEnabledChanged, this,
                [this](bool enabled) {
                    m_syncingMetronomeFromGlobal = true;
                    m_metronomeTrackViewModel->setMute(!enabled);
                    m_syncingMetronomeFromGlobal = false;
                });
        connect(globalAudioContext, &Audio::GlobalAudioContext::metronomeGainChanged, this,
                [this](double gain) {
                    m_syncingMetronomeFromGlobal = true;
                    m_metronomeTrackViewModel->setGain(toDecibels(gain));
                    m_syncingMetronomeFromGlobal = false;
                });
        connect(globalAudioContext, &Audio::GlobalAudioContext::metronomePanChanged, this,
                [this](double pan) {
                    m_syncingMetronomeFromGlobal = true;
                    m_metronomeTrackViewModel->setPan(pan);
                    m_syncingMetronomeFromGlobal = false;
                });

        connect(m_metronomeTrackViewModel, &sflow::TrackViewModel::muteChanged, this, [this] {
            if (!m_syncingMetronomeFromGlobal) {
                Audio::GlobalAudioContext::setMetronomeEnabled(!m_metronomeTrackViewModel->isMute());
            }
        });
        connect(m_metronomeTrackViewModel, &sflow::TrackViewModel::gainChanged, this, [this] {
            if (!m_syncingMetronomeFromGlobal) {
                Audio::GlobalAudioContext::setMetronomeGain(toGain(m_metronomeTrackViewModel->gain()));
            }
        });
        connect(m_metronomeTrackViewModel, &sflow::TrackViewModel::panChanged, this, [this] {
            if (!m_syncingMetronomeFromGlobal) {
                Audio::GlobalAudioContext::setMetronomePan(m_metronomeTrackViewModel->pan());
            }
        });

        m_deviceOutputTrackViewModel = new sflow::TrackViewModel(this);
        m_deviceOutputTrackViewModel->setName(tr("Device Output"));
        m_deviceOutputTrackViewModel->setGain(toDecibels(Audio::GlobalAudioContext::deviceGain()));
        m_deviceOutputTrackViewModel->setPan(Audio::GlobalAudioContext::devicePan());
        m_deviceOutputTrackViewModel->setMultiChannelOutput(false);

        connect(globalAudioContext, &Audio::GlobalAudioContext::deviceGainChanged, this,
                [this](double gain) {
                    m_syncingDeviceOutputFromGlobal = true;
                    m_deviceOutputTrackViewModel->setGain(toDecibels(gain));
                    m_syncingDeviceOutputFromGlobal = false;
                });
        connect(globalAudioContext, &Audio::GlobalAudioContext::devicePanChanged, this,
                [this](double pan) {
                    m_syncingDeviceOutputFromGlobal = true;
                    m_deviceOutputTrackViewModel->setPan(pan);
                    m_syncingDeviceOutputFromGlobal = false;
                });

        connect(m_deviceOutputTrackViewModel, &sflow::TrackViewModel::gainChanged, this, [this] {
            if (!m_syncingDeviceOutputFromGlobal) {
                Audio::GlobalAudioContext::setDeviceGain(toGain(m_deviceOutputTrackViewModel->gain()));
            }
        });
        connect(m_deviceOutputTrackViewModel, &sflow::TrackViewModel::panChanged, this, [this] {
            if (!m_syncingDeviceOutputFromGlobal) {
                Audio::GlobalAudioContext::setDevicePan(m_deviceOutputTrackViewModel->pan());
            }
        });

        m_masterTrackListViewModel->insertItem(m_masterTrackListViewModel->count(), m_metronomeTrackViewModel);
        m_masterTrackListViewModel->insertItem(m_masterTrackListViewModel->count(), m_deviceOutputTrackViewModel);
    }

    void MasterTrackAddOn::extensionsInitialized() {
    }

    bool MasterTrackAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    MasterTrackAddOn *MasterTrackAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<MasterTrackAddOn>();
    }

    sflow::TrackViewModel *MasterTrackAddOn::metronomeTrackViewModel() const {
        return m_metronomeTrackViewModel;
    }

    sflow::TrackViewModel *MasterTrackAddOn::deviceOutputTrackViewModel() const {
        return m_deviceOutputTrackViewModel;
    }

}

#include "moc_MasterTrackAddOn.cpp"
