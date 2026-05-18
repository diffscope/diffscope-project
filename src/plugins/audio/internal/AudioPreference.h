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
        Q_PROPERTY(AudioPreference::PlaybackTogglingAction playbackTogglingAction READ playbackTogglingAction WRITE setPlaybackTogglingAction NOTIFY playbackTogglingActionChanged)
        Q_PROPERTY(bool audioExporterClippingCheckEnabled READ audioExporterClippingCheckEnabled WRITE setAudioExporterClippingCheckEnabled NOTIFY audioExporterClippingCheckEnabledChanged)
        Q_PROPERTY(bool audioExporterEnableAdvancedOptions READ audioExporterEnableAdvancedOptions WRITE setAudioExporterEnableAdvancedOptions NOTIFY audioExporterEnableAdvancedOptionsChanged)
        Q_PROPERTY(bool audioExporterUseTemporaryFile READ audioExporterUseTemporaryFile WRITE setAudioExporterUseTemporaryFile NOTIFY audioExporterUseTemporaryFileChanged)

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

        enum PlaybackTogglingAction {
            PTA_PlayStop,
            PTA_PlayPause,
        };
        Q_ENUM(PlaybackTogglingAction)

        static PlayheadBehavior playbackBehavior();
        static void setPlaybackBehavior(PlayheadBehavior playbackBehavior);
        static PlaybackTogglingAction playbackTogglingAction();
        static void setPlaybackTogglingAction(PlaybackTogglingAction playbackTogglingAction);
        static bool audioExporterClippingCheckEnabled();
        static void setAudioExporterClippingCheckEnabled(bool enabled);
        static bool audioExporterEnableAdvancedOptions();
        static void setAudioExporterEnableAdvancedOptions(bool enabled);
        static bool audioExporterUseTemporaryFile();
        static void setAudioExporterUseTemporaryFile(bool enabled);

    Q_SIGNALS:
        void playbackBehaviorChanged();
        void playbackTogglingActionChanged();
        void audioExporterClippingCheckEnabledChanged();
        void audioExporterEnableAdvancedOptionsChanged();
        void audioExporterUseTemporaryFileChanged();

    private:
        friend class AudioPlugin;
        explicit AudioPreference(QObject *parent = nullptr);
        QScopedPointer<AudioPreferencePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOPREFERENCE_H
