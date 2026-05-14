#ifndef DIFFSCOPE_AUDIO_AUDIOCLIPADDON_H
#define DIFFSCOPE_AUDIO_AUDIOCLIPADDON_H

#include <QString>
#include <QVariant>
#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>
#include <dspxmodel/AudioClip.h>

namespace talcs {
    class AbstractAudioFormatIO;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace Audio::Internal {

    class AudioClipAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit AudioClipAddOn(QObject *parent = nullptr);
        ~AudioClipAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static AudioClipAddOn *of(Core::ProjectWindowInterface *windowHandle);

        Q_INVOKABLE void insertAudioClip();
        Q_INVOKABLE void updateAudioClipToIdenticallyMovedPath(dspx::AudioClip *clip, const QString &filePath);
        Q_INVOKABLE void updateAudioClipDigest(dspx::AudioClip *clip);
        Q_INVOKABLE void replaceAudioClip(dspx::AudioClip *clip);

    private:
        talcs::AbstractAudioFormatIO *openAudioFile(QString *fileName, QVariant *userData, QString *entryClassName);
        dspx::AudioPathInfo audioPathFromFile(const QString &filePath, const QVariant &userData, const QString &entryClassName) const;
        void updateAudioPathLocation(dspx::AudioPathInfo *path, const QString &filePath) const;
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOCLIPADDON_H
