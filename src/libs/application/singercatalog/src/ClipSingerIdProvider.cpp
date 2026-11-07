// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ClipSingerIdProvider.h"
#include "ClipSingerIdProvider_p.h"

#include <utility>

namespace SingerCatalog {

    void ClipSingerIdProviderPrivate::disconnectSources() {
        for (const auto &connection : std::as_const(sourceConnections))
            QObject::disconnect(connection);
        sourceConnections.clear();
    }

    void ClipSingerIdProviderPrivate::disconnectSingerTree() {
        for (const auto &connection : std::as_const(singerTreeConnections))
            QObject::disconnect(connection);
        singerTreeConnections.clear();
    }

    void ClipSingerIdProviderPrivate::rebuildSingerTreeBindings() {
        disconnectSingerTree();
        if (sources)
            bindSingerList(sources->singers());
        updateSingerTree();
    }

    void ClipSingerIdProviderPrivate::bindSingerList(dspx::SingerList *singerList) {
        Q_Q(ClipSingerIdProvider);
        singerTreeConnections.append(QObject::connect(singerList, &dspx::SingerList::itemsChanged, q, [this] {
            rebuildSingerTreeBindings();
        }));

        for (auto *singer : singerList->items()) {
            switch (singer->kind()) {
                case dspx::Singer::Single: {
                    auto *singleSinger = static_cast<dspx::SingleSinger *>(singer);
                    singerTreeConnections.append(
                        QObject::connect(singleSinger, &dspx::SingleSinger::idChanged, q, [this] {
                            updateSingerTree();
                        }));
                    break;
                }
                case dspx::Singer::Mixed: {
                    auto *mixedSinger = static_cast<dspx::MixedSinger *>(singer);
                    bindSingerList(mixedSinger->singers());
                    break;
                }
            }
        }
    }

    void ClipSingerIdProviderPrivate::updateArchitectureId() {
        Q_Q(ClipSingerIdProvider);
        const QString newArchitectureId = sources ? sources->category() : QString{};
        if (architectureId == newArchitectureId)
            return;
        architectureId = newArchitectureId;
        Q_EMIT q->architectureIdChanged(architectureId);
    }

    void ClipSingerIdProviderPrivate::updateSingerTree() {
        Q_Q(ClipSingerIdProvider);
        const QVariantList newSingerTree = sources ? singerListToVariantList(sources->singers()) : QVariantList{};
        if (singerTree == newSingerTree)
            return;
        singerTree = newSingerTree;
        Q_EMIT q->singerTreeChanged(singerTree);
    }

    QVariant ClipSingerIdProviderPrivate::singerToVariant(dspx::Singer *singer) const {
        switch (singer->kind()) {
            case dspx::Singer::Single: {
                const auto *singleSinger = static_cast<dspx::SingleSinger *>(singer);
                return singleSinger->id();
            }
            case dspx::Singer::Mixed: {
                const auto *mixedSinger = static_cast<dspx::MixedSinger *>(singer);
                return singerListToVariantList(mixedSinger->singers());
            }
        }
        return {};
    }

    QVariantList ClipSingerIdProviderPrivate::singerListToVariantList(dspx::SingerList *singerList) const {
        QVariantList result;
        const auto singers = singerList->items();
        result.reserve(singers.size());
        for (auto *singer : singers)
            result.append(singerToVariant(singer));
        return result;
    }

    ClipSingerIdProvider::ClipSingerIdProvider(QObject *parent)
        : QObject(parent), d_ptr(new ClipSingerIdProviderPrivate) {
        Q_D(ClipSingerIdProvider);
        d->q_ptr = this;
    }

    ClipSingerIdProvider::~ClipSingerIdProvider() = default;

    dspx::Sources *ClipSingerIdProvider::sources() const {
        Q_D(const ClipSingerIdProvider);
        return d->sources;
    }

    void ClipSingerIdProvider::setSources(dspx::Sources *sources) {
        Q_D(ClipSingerIdProvider);
        if (d->sources == sources)
            return;

        d->disconnectSources();
        d->disconnectSingerTree();
        d->sources = sources;
        if (sources) {
            d->sourceConnections.append(connect(sources, &dspx::Sources::categoryChanged, this, [d] {
                d->updateArchitectureId();
            }));
            d->sourceConnections.append(connect(sources, &QObject::destroyed, this, [this] {
                Q_D(ClipSingerIdProvider);
                d->disconnectSources();
                d->disconnectSingerTree();
                d->sources = nullptr;
                Q_EMIT sourcesChanged(nullptr);
                d->updateArchitectureId();
                d->updateSingerTree();
            }));
        }

        Q_EMIT sourcesChanged(sources);
        d->updateArchitectureId();
        d->rebuildSingerTreeBindings();
    }

    QString ClipSingerIdProvider::architectureId() const {
        Q_D(const ClipSingerIdProvider);
        return d->architectureId;
    }

    QVariantList ClipSingerIdProvider::singerTree() const {
        Q_D(const ClipSingerIdProvider);
        return d->singerTree;
    }

}

#include "moc_ClipSingerIdProvider.cpp"
