#include "DefaultLyricManager.h"
#include "DefaultLyricManager_p.h"

#include <dspxmodel/SingingClip.h>

namespace Core {

    DefaultLyricManager::DefaultLyricManager(QObject *parent)
        : QObject(parent), d_ptr(new DefaultLyricManagerPrivate) {
        Q_D(DefaultLyricManager);
        d->q_ptr = this;
    }

    DefaultLyricManager::~DefaultLyricManager() = default;

    QString DefaultLyricManager::getDefaultLyricForSingingClip(dspx::SingingClip *targetSingingClip) const {
        Q_UNUSED(targetSingingClip);
        // TODO: Implement default lyric retrieval logic
        return "a";
    }

}

#include "moc_DefaultLyricManager.cpp"
