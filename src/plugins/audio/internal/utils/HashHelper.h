#ifndef DIFFSCOPE_AUDIO_HASHHELPER_H
#define DIFFSCOPE_AUDIO_HASHHELPER_H

#include <QString>

namespace Audio::Internal {

    class HashHelper {
    public:
        static QString sha512(const QString &filePath);
    };

}

#endif // DIFFSCOPE_AUDIO_HASHHELPER_H
