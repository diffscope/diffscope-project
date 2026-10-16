// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DefaultLyricManager.h"
#include "DefaultLyricManager_p.h"

#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/SingerInfo.h>
#include <coreplugin/SingerRegistry.h>
#include <coreplugin/internal/BehaviorPreference.h>

namespace Core {

    namespace {

        dspx::SingleSinger *firstLeafSinger(dspx::SingerList *singers) {
            if (!singers)
                return nullptr;

            for (auto *singer : singers->items()) {
                if (!singer)
                    continue;
                if (singer->kind() == dspx::Singer::Single) {
                    auto *singleSinger = qobject_cast<dspx::SingleSinger *>(singer);
                    if (singleSinger && !singleSinger->id().isEmpty())
                        return singleSinger;
                    continue;
                }

                auto *mixedSinger = qobject_cast<dspx::MixedSinger *>(singer);
                if (!mixedSinger)
                    continue;
                if (auto *leafSinger = firstLeafSinger(mixedSinger->singers()))
                    return leafSinger;
            }
            return nullptr;
        }

        QString defaultLyricForSingingClip(dspx::SingingClip *targetSingingClip) {
            const auto fallback = Internal::BehaviorPreference::fallbackLyricText();
            if (!targetSingingClip || !targetSingingClip->sources())
                return fallback;

            const auto *leafSinger = firstLeafSinger(targetSingingClip->sources()->singers());
            auto *registry = CoreInterface::singerRegistry();
            if (!leafSinger || !registry)
                return fallback;

            const auto singerInfo = registry->singerInfo(targetSingingClip->sources()->category(), leafSinger->id());
            const auto languages = singerInfo.languages();
            if (languages.isEmpty())
                return fallback;

            const auto languageIt = singerInfo.defaultLanguage().isEmpty()
                                        ? languages.cbegin()
                                        : languages.constFind(singerInfo.defaultLanguage());
            if (languageIt == languages.cend())
                return fallback;

            return languageIt->defaultLyric.isEmpty() ? fallback : languageIt->defaultLyric;
        }

    }

    DefaultLyricManager::DefaultLyricManager(QObject *parent)
        : QObject(parent), d_ptr(new DefaultLyricManagerPrivate) {
        Q_D(DefaultLyricManager);
        d->q_ptr = this;
    }

    DefaultLyricManager::~DefaultLyricManager() = default;

    QString DefaultLyricManager::getDefaultLyricForSingingClip(dspx::SingingClip *targetSingingClip) const {
        return defaultLyricForSingingClip(targetSingingClip);
    }

}

#include "moc_DefaultLyricManager.cpp"
