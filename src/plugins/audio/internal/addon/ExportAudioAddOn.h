#ifndef DIFFSCOPE_AUDIO_EXPORTAUDIOADDON_H
#define DIFFSCOPE_AUDIO_EXPORTAUDIOADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace Audio::Internal {

    class ExportAudioAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit ExportAudioAddOn(QObject *parent = nullptr);
        ~ExportAudioAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static ExportAudioAddOn *of(Core::ProjectWindowInterface *windowHandle);

        Q_INVOKABLE void exportAudio();
        Q_INVOKABLE QStringList formatOptions(int fileType) const;
        Q_INVOKABLE void browseFile();
        Q_INVOKABLE void setMixingOption(int index);
        Q_INVOKABLE void setFileType(int index);
    };

}

#endif // DIFFSCOPE_AUDIO_EXPORTAUDIOADDON_H
