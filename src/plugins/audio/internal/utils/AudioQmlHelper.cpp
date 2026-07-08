#include "AudioQmlHelper.h"

#include <QDir>

#include <dspxmodelORM/AudioClip.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <audio/AudioClipAudioContext.h>
#include <audio/internal/AudioClipAddOn.h>

namespace Audio::Internal {

    AudioQmlHelper::AudioQmlHelper(QObject *parent) : QObject(parent) {
    }

    AudioQmlHelper::~AudioQmlHelper() = default;

    QString AudioQmlHelper::getDisplayAudioFilePath(const dspx::AudioPathInfo &pathInfo) {
        if (pathInfo.absoluteDir.isEmpty() || pathInfo.fileName.isEmpty()) {
            return {};
        }
        return QDir::toNativeSeparators(QDir(pathInfo.absoluteDir).filePath(pathInfo.fileName));
    }

    AudioClipAudioContext *AudioQmlHelper::getAudioClipAudioContext(QObject *object) {
        auto audioClip = qobject_cast<dspx::AudioClip *>(object);
        if (!audioClip) {
            return nullptr;
        }
        return AudioClipAudioContext::of(audioClip);
    }

    QString AudioQmlHelper::getNativeSeparatorPath(const QString &path) {
        return QDir::toNativeSeparators(path);
    }

    AudioClipAddOn *AudioQmlHelper::getAudioClipAddOn(Core::ProjectWindowInterface *win) {
        return AudioClipAddOn::of(win);
    }

}

#include "moc_AudioQmlHelper.cpp"
