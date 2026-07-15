#ifndef DIFFSCOPE_AUDIO_AUDIOSYSTEM_H
#define DIFFSCOPE_AUDIO_AUDIOSYSTEM_H

#include <QObject>

namespace Audio::Internal {

    class OutputSystem;

    class AudioSystem : public QObject {
        Q_OBJECT
    public:
        explicit AudioSystem(QObject *parent = nullptr);
        ~AudioSystem() override;

        static AudioSystem *instance();

        static OutputSystem *outputSystem();

        static bool isSingerRegistrationSuccessful();

    private:
        OutputSystem *m_outputSystem;
        bool m_singerRegistrationSuccessful{};
    };

}

#endif //DIFFSCOPE_AUDIO_AUDIOSYSTEM_H
