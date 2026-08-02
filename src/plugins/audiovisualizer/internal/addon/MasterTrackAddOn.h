#ifndef DIFFSCOPE_AUDIO_VISUALIZER_MASTERTRACKADDON_H
#define DIFFSCOPE_AUDIO_VISUALIZER_MASTERTRACKADDON_H

#include <CoreApi/windowinterface.h>

#include <QPointer>

namespace Core {
    class ProjectWindowInterface;
}

namespace sflow {
    class ListViewModel;
    class TrackViewModel;
}

namespace AudioVisualizer::Internal {

    class MasterTrackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit MasterTrackAddOn(QObject *parent = nullptr);
        ~MasterTrackAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static MasterTrackAddOn *of(Core::ProjectWindowInterface *windowHandle);
        sflow::TrackViewModel *metronomeTrackViewModel() const;
        sflow::TrackViewModel *deviceOutputTrackViewModel() const;

    private:
        QPointer<sflow::ListViewModel> m_masterTrackListViewModel;
        QPointer<sflow::TrackViewModel> m_metronomeTrackViewModel;
        QPointer<sflow::TrackViewModel> m_deviceOutputTrackViewModel;
        bool m_syncingMetronomeFromGlobal{};
        bool m_syncingDeviceOutputFromGlobal{};
    };

}

#endif // DIFFSCOPE_AUDIO_VISUALIZER_MASTERTRACKADDON_H
