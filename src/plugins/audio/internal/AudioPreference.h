#ifndef DIFFSCOPE_AUDIO_AUDIOPREFERENCE_H
#define DIFFSCOPE_AUDIO_AUDIOPREFERENCE_H

#include <qqmlintegration.h>

#include <QObject>

class QQmlEngine;
class QJSEngine;

namespace Audio::Internal {

    class AudioPlugin;

    class AudioPreferencePrivate;

    class AudioPreference : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(AudioPreference)
        Q_PROPERTY(AudioPreference::PlayheadBehavior playbackBehavior READ playbackBehavior WRITE setPlaybackBehavior NOTIFY playbackBehaviorChanged)

    public:
        ~AudioPreference() override;

        static AudioPreference *instance();

        static inline AudioPreference *create(QQmlEngine *, QJSEngine *) {
            return instance();
        }

        void load();
        void save() const;

        enum PlayheadBehavior {
            PB_ReturnToStart,
            PB_KeepAtCurrent,
            PB_KeepAtCurrentButPlayFromStart,
        };
        Q_ENUM(PlayheadBehavior)

        static PlayheadBehavior playbackBehavior();
        static void setPlaybackBehavior(PlayheadBehavior playbackBehavior);

    Q_SIGNALS:
        void playbackBehaviorChanged();

    private:
        friend class AudioPlugin;
        explicit AudioPreference(QObject *parent = nullptr);
        QScopedPointer<AudioPreferencePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOPREFERENCE_H
