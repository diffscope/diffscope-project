#ifndef DIFFSCOPE_AUDIO_INSERTAUDIOCLIPADDON_H
#define DIFFSCOPE_AUDIO_INSERTAUDIOCLIPADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace Audio::Internal {

    class InsertAudioClipAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit InsertAudioClipAddOn(QObject *parent = nullptr);
        ~InsertAudioClipAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static InsertAudioClipAddOn *of(Core::ProjectWindowInterface *windowHandle);

        Q_INVOKABLE void insertAudioClip();
    };

}

#endif // DIFFSCOPE_AUDIO_INSERTAUDIOCLIPADDON_H
