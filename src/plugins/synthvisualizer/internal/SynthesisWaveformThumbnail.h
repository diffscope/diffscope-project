#ifndef DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISWAVEFORMTHUMBNAIL_H
#define DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISWAVEFORMTHUMBNAIL_H

#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QString>
#include <qqmlintegration.h>

#include <SVSCraftQuick/WaveformThumbnail.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace SynthVisualizer::Internal {

    class SynthesisWaveformThumbnail : public SVS::WaveformThumbnail {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QString sourceFilePath READ sourceFilePath WRITE setSourceFilePath NOTIFY sourceFilePathChanged)
        Q_PROPERTY(Core::ProjectWindowInterface *projectWindowInterface READ projectWindowInterface WRITE setProjectWindowInterface NOTIFY projectWindowInterfaceChanged)
        Q_PROPERTY(double startTick READ startTick WRITE setStartTick NOTIFY startTickChanged)
        Q_PROPERTY(double durationTicks READ durationTicks WRITE setDurationTicks NOTIFY durationTicksChanged)
        Q_PROPERTY(double verticalScaleFactor READ verticalScaleFactor NOTIFY verticalScaleFactorChanged)

    public:
        explicit SynthesisWaveformThumbnail(QQuickItem *parent = nullptr);
        ~SynthesisWaveformThumbnail() override;

        QString sourceFilePath() const;
        void setSourceFilePath(const QString &sourceFilePath);

        Core::ProjectWindowInterface *projectWindowInterface() const;
        void setProjectWindowInterface(Core::ProjectWindowInterface *projectWindowInterface);

        double startTick() const;
        void setStartTick(double startTick);

        double durationTicks() const;
        void setDurationTicks(double durationTicks);

        double verticalScaleFactor() const;

    Q_SIGNALS:
        void sourceFilePathChanged();
        void projectWindowInterfaceChanged();
        void startTickChanged();
        void durationTicksChanged();
        void verticalScaleFactorChanged();

    private:
        void loadAudioFile();
        void clearWaveform();
        void setVerticalScaleFactor(double verticalScaleFactor);
        void reconnectProjectWindowInterface();
        void updateWaveformGeometry();
        double tickToMillisecond(double tick) const;
        double tickRangeToSamples(double startTick, double endTick) const;

        QString m_audioFilePath;
        QPointer<Core::ProjectWindowInterface> m_projectWindowInterface;
        QList<QMetaObject::Connection> m_projectWindowInterfaceConnections;
        double m_startTick{};
        double m_durationTicks{};
        double m_sampleRate{};
        double m_verticalScaleFactor{1.0};
        quint64 m_loadRevision{};
    };

}

#endif // DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISWAVEFORMTHUMBNAIL_H
