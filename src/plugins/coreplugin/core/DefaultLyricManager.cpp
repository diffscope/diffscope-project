// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DefaultLyricManager.h"

#include <QPair>

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
#include <coreplugin/private/DefaultLyricManager_p.h>

namespace Core {

    namespace {

        dspx::SingleSinger *firstLeafSinger(dspx::SingerList *singers) {
            if (!singers)
                return nullptr;

            for (auto *singer : singers->items()) {
                if (!singer)
                    continue;
                if (singer->type() == dspx::Singer::Single) {
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

        QPair<QString, QString> fallbackDefaults() {
            return {
                Internal::BehaviorPreference::fallbackLyricLanguageCode(),
                Internal::BehaviorPreference::fallbackLyricText(),
            };
        }

        QPair<QString, QString> defaultsForSingingClip(dspx::SingingClip *targetSingingClip) {
            const auto fallback = fallbackDefaults();
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

            auto languageIt = singerInfo.defaultLanguage().isEmpty()
                                  ? languages.cend()
                                  : languages.constFind(singerInfo.defaultLanguage());
            if (languageIt == languages.cend())
                languageIt = languages.cbegin();

            return {
                languageIt.key(),
                languageIt->defaultLyric.isEmpty() ? fallback.second : languageIt->defaultLyric,
            };
        }

    }

    DefaultLyricManager::DefaultLyricManager(QObject *parent)
        : QObject(parent), d_ptr(new DefaultLyricManagerPrivate) {
        Q_D(DefaultLyricManager);
        d->q_ptr = this;
    }

    DefaultLyricManager::~DefaultLyricManager() = default;

    QString DefaultLyricManager::getDefaultLyricForSingingClip(dspx::SingingClip *targetSingingClip) const {
        return defaultsForSingingClip(targetSingingClip).second;
    }

    QString DefaultLyricManager::getDefaultLanguageForSingingClip(dspx::SingingClip *targetSingingClip) const {
        return defaultsForSingingClip(targetSingingClip).first;
    }

}

#include "moc_DefaultLyricManager.cpp"
