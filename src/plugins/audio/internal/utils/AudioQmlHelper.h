#ifndef DIFFSCOPE_AUDIO_AUDIOQMLHELPER_H
#define DIFFSCOPE_AUDIO_AUDIOQMLHELPER_H

#include <qqmlintegration.h>

#include <QObject>
#include <QString>

namespace dspx {
    class AudioClip;
    struct AudioPathInfo;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace Audio {
    class AudioClipAudioContext;
}

namespace Audio::Internal {

    class AudioClipAddOn;

    class AudioQmlHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

    public:
        explicit AudioQmlHelper(QObject *parent = nullptr);
        ~AudioQmlHelper() override;

        Q_INVOKABLE static QString getDisplayAudioFilePath(const dspx::AudioPathInfo &pathInfo);
        Q_INVOKABLE static AudioClipAudioContext *getAudioClipAudioContext(QObject *object);
        Q_INVOKABLE static QString getNativeSeparatorPath(const QString &path);
        Q_INVOKABLE static AudioClipAddOn *getAudioClipAddOn(Core::ProjectWindowInterface *win);
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOQMLHELPER_H
