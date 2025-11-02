#ifndef DIFFSCOPE_AUDIO_OUTPUTSYSTEM_H
#define DIFFSCOPE_AUDIO_OUTPUTSYSTEM_H

#include <memory>

#include <QLoggingCategory>

#include <TalcsDevice/OutputContext.h>

namespace Audio::Internal {

    class OutputSystem : public QObject {
        Q_OBJECT
    public:
        explicit OutputSystem(QObject *parent = nullptr);
        ~OutputSystem() override;

        bool initialize();

        talcs::OutputContext *outputContext() const;

        bool setDriver(const QString &driverName);
        bool setDevice(const QString &deviceName);
        bool setAdoptedBufferSize(qint64 bufferSize);
        bool setAdoptedSampleRate(double sampleRate);
        void setHotPlugNotificationMode(talcs::OutputContext::HotPlugNotificationMode mode);

        bool isReady() const;

    private:
        void load();
        void save() const;

        void postSetDevice();

        void logOutputInfo() const;

        std::unique_ptr<talcs::OutputContext> m_outputContext;

        qint64 m_adoptedBufferSize{};
        double m_adoptedSampleRate{};
        double m_deviceGain{};
        double m_devicePan{};
        talcs::OutputContext::HotPlugNotificationMode m_hotPlugNotificationMode{};
        QString m_driverName;
        QString m_deviceName;
    };

}

#endif //DIFFSCOPE_AUDIO_OUTPUTSYSTEM_H
