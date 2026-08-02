#ifndef DIFFSCOPE_AUDIO_VISUALIZER_LEVELMETERADDON_H
#define DIFFSCOPE_AUDIO_VISUALIZER_LEVELMETERADDON_H

#include <CoreApi/windowinterface.h>

#include <QElapsedTimer>
#include <QHash>

class QTimer;

namespace dspx {
    class Track;
}

namespace Audio {
    class ProjectAudioContext;
}

namespace VisualEditor {
    class ProjectViewModelContext;
}

namespace AudioVisualizer::Internal {

    class LevelMeterAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit LevelMeterAddOn(QObject *parent = nullptr);
        ~LevelMeterAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private Q_SLOTS:
        void addTrack(int index, dspx::Track *track);
        void removeTrack(int index, dspx::Track *track);
        void tickLevelMeters();

    private:
        struct MeterState;

        void bindMasterTrack();
        void bindMetronomeTrack();
        void bindDeviceOutputTrack();
        void startLevelMeterTimer();

        Audio::ProjectAudioContext *m_audioContext{};
        VisualEditor::ProjectViewModelContext *m_viewModelContext{};
        QHash<dspx::Track *, MeterState *> m_trackMeters;
        MeterState *m_masterMeter{};
        MeterState *m_metronomeMeter{};
        MeterState *m_deviceOutputMeter{};
        QTimer *m_levelMeterTimer{};
        QElapsedTimer m_levelMeterTickTime;
        bool m_levelMeterActive{};
    };

}

#endif // DIFFSCOPE_AUDIO_VISUALIZER_LEVELMETERADDON_H
