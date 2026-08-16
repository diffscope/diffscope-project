// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LevelMeterAddOn.h"

#include <QPointer>
#include <QTimer>
#include <QVector>

#include <algorithm>
#include <utility>

#include <ScopicFlowCore/ListViewModel.h>
#include <ScopicFlowCore/TrackListInteractionController.h>
#include <ScopicFlowCore/TrackViewModel.h>

#include <SVSCraftCore/DecibelLinearizer.h>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsCore/SmoothedFloat.h>

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/TrackAudioContext.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>
#include <visualeditor/ArrangementPanelInterface.h>
#include <visualeditor/MixerPanelInterface.h>

#include <audiovisualizer/internal/MasterTrackAddOn.h>

namespace AudioVisualizer::Internal {

    namespace {

        constexpr float LevelFloor = -96.0f;
        constexpr int LevelMeterRampLength = 128;
        constexpr int LevelMeterRefreshInterval = 8;

        void updateSmoothedValue(talcs::SmoothedFloat &smoothedValue, float decibels) {
            if (decibels < smoothedValue.currentValue()) {
                smoothedValue.setTargetValue(decibels);
            } else {
                smoothedValue.setCurrentAndTargetValue(decibels);
            }
        }

        void resetPeakAndClipping(sflow::TrackViewModel *viewModel) {
            if (!viewModel) {
                return;
            }
            viewModel->setLeftClipping(false);
            viewModel->setRightClipping(false);
            viewModel->setPeakLevel(LevelFloor);
        }

        void bindLevelMeterReset(sflow::TrackListInteractionController *controller,
                                 sflow::ListViewModel *viewModelList, QObject *context) {
            Q_ASSERT(controller);
            Q_ASSERT(viewModelList);
            if (!controller || !viewModelList) {
                return;
            }
            QObject::connect(controller, &sflow::TrackListInteractionController::itemLevelMeterClicked,
                             context, [viewModelList](QQuickItem *, int index) {
                                 const auto items = viewModelList->items();
                                 resetPeakAndClipping(qobject_cast<sflow::TrackViewModel *>(items.value(index)));
                             });
        }

    }

    struct LevelMeterAddOn::MeterState {
        MeterState(QObject *source, sflow::TrackViewModel *viewModel,
                   bool resetWhenProjectStopped = true)
            : source(source), viewModel(viewModel), left(LevelFloor), right(LevelFloor),
              resetWhenProjectStopped(resetWhenProjectStopped) {
            left.setRampLength(LevelMeterRampLength);
            right.setRampLength(LevelMeterRampLength);
        }

        QPointer<QObject> source;
        QPointer<sflow::TrackViewModel> viewModel;
        talcs::SmoothedFloat left;
        talcs::SmoothedFloat right;
        bool resetWhenProjectStopped;
    };

    LevelMeterAddOn::LevelMeterAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
        m_levelMeterTimer = new QTimer(this);
        m_levelMeterTimer->setSingleShot(true);
        connect(m_levelMeterTimer, &QTimer::timeout, this, &LevelMeterAddOn::tickLevelMeters);
    }

    LevelMeterAddOn::~LevelMeterAddOn() {
        qDeleteAll(m_trackMeters);
        delete m_masterMeter;
        delete m_metronomeMeter;
        delete m_deviceOutputMeter;
    }

    void LevelMeterAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        m_audioContext = Audio::ProjectAudioContext::of(windowInterface);
        m_viewModelContext = VisualEditor::ProjectViewModelContext::of(windowInterface);
        Q_ASSERT(m_audioContext);
        Q_ASSERT(m_viewModelContext);
        if (!m_audioContext || !m_viewModelContext) {
            return;
        }

        bindMasterTrack();
        bindMetronomeTrack();
        bindDeviceOutputTrack();

        auto trackList = windowInterface->projectDocumentContext()->document()->model()->tracks();
        const auto tracks = trackList->items();
        for (int i = 0; i < tracks.size(); ++i) {
            addTrack(i, tracks.at(i));
        }
        connect(trackList, &dspx::TrackList::itemInserted, this, &LevelMeterAddOn::addTrack);
        connect(trackList, &dspx::TrackList::itemRemoved, this, &LevelMeterAddOn::removeTrack);

        auto arrangementPanelInterface = VisualEditor::ArrangementPanelInterface::of(windowInterface);
        auto mixerPanelInterface = VisualEditor::MixerPanelInterface::of(windowInterface);
        Q_ASSERT(arrangementPanelInterface);
        Q_ASSERT(mixerPanelInterface);
        if (arrangementPanelInterface) {
            bindLevelMeterReset(arrangementPanelInterface->trackListInteractionController(),
                                m_viewModelContext->trackListViewModel(), this);
        }
        if (mixerPanelInterface) {
            bindLevelMeterReset(mixerPanelInterface->trackListInteractionController(),
                                m_viewModelContext->trackListViewModel(), this);
            bindLevelMeterReset(mixerPanelInterface->masterTrackListInteractionController(),
                                m_viewModelContext->masterTrackListViewModel(), this);
        }

        connect(m_audioContext, &Audio::ProjectAudioContext::statusChanged, this,
                [this](Audio::ProjectAudioContext::PlaybackStatus status) {
                    if (status == Audio::ProjectAudioContext::Playing) {
                        startLevelMeterTimer();
                    }
                });
        if (m_audioContext->status() == Audio::ProjectAudioContext::Playing) {
            startLevelMeterTimer();
        }
    }

    void LevelMeterAddOn::extensionsInitialized() {
    }

    bool LevelMeterAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    void LevelMeterAddOn::addTrack(int index, dspx::Track *track) {
        Q_UNUSED(index)
        if (!track || m_trackMeters.contains(track)) {
            return;
        }

        auto audioContext = Audio::TrackAudioContext::of(track);
        auto viewModel = m_viewModelContext->getTrackViewItemFromDocumentItem(track);
        Q_ASSERT(audioContext);
        Q_ASSERT(viewModel);
        if (!audioContext || !viewModel) {
            return;
        }

        auto source = audioContext->controlMixer();
        auto state = new MeterState(source, viewModel);
        m_trackMeters.insert(track, state);
        viewModel->setLeftLevel(LevelFloor);
        viewModel->setRightLevel(LevelFloor);
        viewModel->setPeakLevel(LevelFloor);

        source->setLevelMeterChannelCount(2);
        connect(source, &talcs::PositionableMixerAudioSource::levelMetered, this,
                [this, track, source](const QVector<float> &values) {
                    auto state = m_trackMeters.value(track);
                    if (!state || state->source.data() != source) {
                        return;
                    }

                    float left = values.value(0, 0.0f);
                    float right = values.value(1, 0.0f);
                    if (m_audioContext->masterTrackMixer()->isMutedBySoloSetting(source)) {
                        left = 0.0f;
                        right = 0.0f;
                    }

                    updateSmoothedValue(state->left, static_cast<float>(SVS::DecibelLinearizer::gainToDecibels(left)));
                    updateSmoothedValue(state->right, static_cast<float>(SVS::DecibelLinearizer::gainToDecibels(right)));
                    if (m_audioContext->status() == Audio::ProjectAudioContext::Playing) {
                        startLevelMeterTimer();
                    }
                });
    }

    void LevelMeterAddOn::bindMetronomeTrack() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto masterTrackAddOn = MasterTrackAddOn::of(windowInterface);
        auto viewModel = masterTrackAddOn ? masterTrackAddOn->metronomeTrackViewModel() : nullptr;
        auto source = m_audioContext->metronomeControlMixer();
        Q_ASSERT(viewModel);
        Q_ASSERT(source);
        if (!viewModel || !source) {
            return;
        }

        m_metronomeMeter = new MeterState(source, viewModel);
        viewModel->setLeftLevel(LevelFloor);
        viewModel->setRightLevel(LevelFloor);
        viewModel->setPeakLevel(LevelFloor);
        source->setLevelMeterChannelCount(2);
        connect(source, &talcs::MixerAudioSource::levelMetered, this,
                [this](const QVector<float> &values) {
                    if (!m_metronomeMeter) {
                        return;
                    }
                    updateSmoothedValue(m_metronomeMeter->left,
                                        static_cast<float>(SVS::DecibelLinearizer::gainToDecibels(values.value(0, 0.0f))));
                    updateSmoothedValue(m_metronomeMeter->right,
                                        static_cast<float>(SVS::DecibelLinearizer::gainToDecibels(values.value(1, 0.0f))));
                    if (m_audioContext->status() == Audio::ProjectAudioContext::Playing) {
                        startLevelMeterTimer();
                    }
                });
    }

    void LevelMeterAddOn::bindDeviceOutputTrack() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto masterTrackAddOn = MasterTrackAddOn::of(windowInterface);
        auto viewModel = masterTrackAddOn ? masterTrackAddOn->deviceOutputTrackViewModel() : nullptr;
        auto source = Audio::GlobalAudioContext::controlMixer();
        Q_ASSERT(viewModel);
        Q_ASSERT(source);
        if (!viewModel || !source) {
            return;
        }

        m_deviceOutputMeter = new MeterState(source, viewModel, false);
        viewModel->setLeftLevel(LevelFloor);
        viewModel->setRightLevel(LevelFloor);
        viewModel->setPeakLevel(LevelFloor);
        source->setLevelMeterChannelCount(2);
        connect(source, &talcs::MixerAudioSource::levelMetered, this,
                [this](const QVector<float> &values) {
                    if (!m_deviceOutputMeter) {
                        return;
                    }
                    updateSmoothedValue(m_deviceOutputMeter->left,
                                        static_cast<float>(SVS::DecibelLinearizer::gainToDecibels(values.value(0, 0.0f))));
                    updateSmoothedValue(m_deviceOutputMeter->right,
                                        static_cast<float>(SVS::DecibelLinearizer::gainToDecibels(values.value(1, 0.0f))));
                    startLevelMeterTimer();
                });
    }

    void LevelMeterAddOn::removeTrack(int index, dspx::Track *track) {
        Q_UNUSED(index)
        delete m_trackMeters.take(track);
    }

    void LevelMeterAddOn::bindMasterTrack() {
        const auto masterItems = m_viewModelContext->masterTrackListViewModel()->items();
        auto viewModel = qobject_cast<sflow::TrackViewModel *>(masterItems.value(0));
        auto source = m_audioContext->masterControlMixer();
        Q_ASSERT(viewModel);
        Q_ASSERT(source);
        if (!viewModel || !source) {
            return;
        }

        m_masterMeter = new MeterState(source, viewModel);
        viewModel->setLeftLevel(LevelFloor);
        viewModel->setRightLevel(LevelFloor);
        viewModel->setPeakLevel(LevelFloor);
        source->setLevelMeterChannelCount(2);
        connect(source, &talcs::PositionableMixerAudioSource::levelMetered, this,
                [this](const QVector<float> &values) {
                    if (!m_masterMeter) {
                        return;
                    }
                    updateSmoothedValue(m_masterMeter->left,
                                        static_cast<float>(SVS::DecibelLinearizer::gainToDecibels(values.value(0, 0.0f))));
                    updateSmoothedValue(m_masterMeter->right,
                                        static_cast<float>(SVS::DecibelLinearizer::gainToDecibels(values.value(1, 0.0f))));
                    if (m_audioContext->status() == Audio::ProjectAudioContext::Playing) {
                        startLevelMeterTimer();
                    }
                });
    }

    void LevelMeterAddOn::startLevelMeterTimer() {
        if (m_levelMeterActive) {
            return;
        }
        m_levelMeterActive = true;
        m_levelMeterTickTime.start();
        tickLevelMeters();
    }

    void LevelMeterAddOn::tickLevelMeters() {
        const bool notPlaying = m_audioContext->status() != Audio::ProjectAudioContext::Playing;
        bool allAtFloor = true;

        const auto updateViewModel = [notPlaying, &allAtFloor](MeterState *state) {
            if (!state) {
                return;
            }
            if (notPlaying && state->resetWhenProjectStopped) {
                if (state->left.targetValue() > LevelFloor) {
                    state->left.setTargetValue(LevelFloor);
                }
                if (state->right.targetValue() > LevelFloor) {
                    state->right.setTargetValue(LevelFloor);
                }
            }

            const double left = state->left.nextValue();
            const double right = state->right.nextValue();
            if (state->viewModel) {
                state->viewModel->setLeftLevel(left);
                state->viewModel->setRightLevel(right);

                const double peak = std::max(left, right);
                if (peak > state->viewModel->peakLevel()) {
                    state->viewModel->setPeakLevel(peak);
                }
                if (left > 0.0) {
                    state->viewModel->setLeftClipping(true);
                }
                if (right > 0.0) {
                    state->viewModel->setRightClipping(true);
                }
            }

            if (state->left.currentValue() > LevelFloor || state->right.currentValue() > LevelFloor ||
                state->left.isSmoothing() || state->right.isSmoothing()) {
                allAtFloor = false;
            }
        };

        for (auto state : std::as_const(m_trackMeters)) {
            updateViewModel(state);
        }
        updateViewModel(m_masterMeter);
        updateViewModel(m_metronomeMeter);
        updateViewModel(m_deviceOutputMeter);

        if (notPlaying && allAtFloor) {
            m_levelMeterActive = false;
            return;
        }

        const qint64 elapsed = m_levelMeterTickTime.restart();
        m_levelMeterTimer->start(std::max(0, LevelMeterRefreshInterval - static_cast<int>(elapsed)));
    }

}

#include "moc_LevelMeterAddOn.cpp"
