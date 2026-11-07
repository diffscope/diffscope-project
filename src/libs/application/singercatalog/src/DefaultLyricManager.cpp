// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DefaultLyricManager.h"
#include "DefaultLyricManager_p.h"

#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>

#include <singercatalog/SingerInfo.h>
#include <singercatalog/SingerRegistry.h>

namespace SingerCatalog {

    namespace {

        dspx::SingleSinger *firstLeafSinger(dspx::SingerList *singers) {
            for (auto *singer : singers->items()) {
                if (singer->kind() == dspx::Singer::Single) {
                    auto *singleSinger = static_cast<dspx::SingleSinger *>(singer);
                    if (!singleSinger->id().isEmpty())
                        return singleSinger;
                    continue;
                }

                auto *mixedSinger = static_cast<dspx::MixedSinger *>(singer);
                if (auto *leafSinger = firstLeafSinger(mixedSinger->singers()))
                    return leafSinger;
            }
            return nullptr;
        }

    }

    DefaultLyricManager::DefaultLyricManager(const SingerRegistry *singerRegistry, QObject *parent)
        : QObject(parent), d_ptr(new DefaultLyricManagerPrivate{.q_ptr = this, .singerRegistry = singerRegistry}) {
    }

    DefaultLyricManager::~DefaultLyricManager() = default;

    QString DefaultLyricManager::fallback() const {
        Q_D(const DefaultLyricManager);
        return d->fallback;
    }

    void DefaultLyricManager::setFallback(const QString &fallback) {
        Q_D(DefaultLyricManager);
        if (d->fallback == fallback)
            return;
        d->fallback = fallback;
        Q_EMIT fallbackChanged();
    }

    QString DefaultLyricManager::getDefaultLyricForSingingClip(dspx::SingingClip *targetSingingClip) const {
        Q_D(const DefaultLyricManager);
        if (!targetSingingClip->sources())
            return d->fallback;

        const auto *leafSinger = firstLeafSinger(targetSingingClip->sources()->singers());
        if (!leafSinger)
            return d->fallback;

        const auto singerInfo = d->singerRegistry->singerInfo(targetSingingClip->sources()->category(), leafSinger->id());
        const auto languages = singerInfo.languages();
        if (languages.isEmpty())
            return d->fallback;

        const auto languageIt = singerInfo.defaultLanguage().isEmpty()
                                    ? languages.cbegin()
                                    : languages.constFind(singerInfo.defaultLanguage());
        if (languageIt == languages.cend())
            return d->fallback;

        return languageIt->defaultLyric.isEmpty() ? d->fallback : languageIt->defaultLyric;
    }

}

#include "moc_DefaultLyricManager.cpp"
