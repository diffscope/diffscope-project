#include "SingerRegistry.h"
#include "SingerRegistry_p.h"

namespace Core {

    SingerRegistry::SingerRegistry(QObject *parent) : QObject(parent), d_ptr(new SingerRegistryPrivate) {
        Q_D(SingerRegistry);
        d->q_ptr = this;
    }

    SingerRegistry::~SingerRegistry() = default;

    bool SingerRegistry::registerArchitecture(const QString &architectureId, const ArchitectureInfo &info) {
        Q_D(SingerRegistry);
        if (architectureId.isEmpty() || d->architectures.contains(architectureId))
            return false;
        d->architectures.insert(architectureId, {info, {}});
        emit architectureRegistered(architectureId);
        emit architectureIdsChanged(d->architectures.keys());
        return true;
    }

    bool SingerRegistry::updateArchitecture(const QString &architectureId, const ArchitectureInfo &info) {
        Q_D(SingerRegistry);
        auto it = d->architectures.find(architectureId);
        if (architectureId.isEmpty() || it == d->architectures.end())
            return false;
        if (it->info == info)
            return true;
        it->info = info;
        emit architectureUpdated(architectureId);
        return true;
    }

    bool SingerRegistry::removeArchitecture(const QString &architectureId) {
        Q_D(SingerRegistry);
        auto it = d->architectures.find(architectureId);
        if (architectureId.isEmpty() || it == d->architectures.end())
            return false;

        const auto singerIds = it->singers.keys();
        d->architectures.erase(it);
        for (const auto &singerId : singerIds)
            emit singerRemoved(architectureId, singerId);
        emit architectureRemoved(architectureId);
        emit architectureIdsChanged(d->architectures.keys());
        return true;
    }

    bool SingerRegistry::containsArchitecture(const QString &architectureId) const {
        Q_D(const SingerRegistry);
        return !architectureId.isEmpty() && d->architectures.contains(architectureId);
    }

    ArchitectureInfo SingerRegistry::architectureInfo(const QString &architectureId) const {
        Q_D(const SingerRegistry);
        const auto it = d->architectures.constFind(architectureId);
        return it == d->architectures.cend() ? ArchitectureInfo{} : it->info;
    }

    QStringList SingerRegistry::architectureIds() const {
        Q_D(const SingerRegistry);
        return d->architectures.keys();
    }

    bool SingerRegistry::registerSinger(const QString &architectureId, const QString &singerId,
                                        const SingerInfo &info) {
        Q_D(SingerRegistry);
        auto architectureIt = d->architectures.find(architectureId);
        if (architectureId.isEmpty() || singerId.isEmpty() || architectureIt == d->architectures.end() ||
            architectureIt->singers.contains(singerId)) {
            return false;
        }
        architectureIt->singers.insert(singerId, info);
        emit singerRegistered(architectureId, singerId);
        return true;
    }

    bool SingerRegistry::updateSinger(const QString &architectureId, const QString &singerId,
                                      const SingerInfo &info) {
        Q_D(SingerRegistry);
        auto architectureIt = d->architectures.find(architectureId);
        if (architectureId.isEmpty() || singerId.isEmpty() || architectureIt == d->architectures.end())
            return false;
        auto singerIt = architectureIt->singers.find(singerId);
        if (singerIt == architectureIt->singers.end())
            return false;
        if (*singerIt == info)
            return true;
        *singerIt = info;
        emit singerUpdated(architectureId, singerId);
        return true;
    }

    bool SingerRegistry::removeSinger(const QString &architectureId, const QString &singerId) {
        Q_D(SingerRegistry);
        auto architectureIt = d->architectures.find(architectureId);
        if (architectureId.isEmpty() || singerId.isEmpty() || architectureIt == d->architectures.end() ||
            !architectureIt->singers.remove(singerId)) {
            return false;
        }
        emit singerRemoved(architectureId, singerId);
        return true;
    }

    bool SingerRegistry::containsSinger(const QString &architectureId, const QString &singerId) const {
        Q_D(const SingerRegistry);
        const auto architectureIt = d->architectures.constFind(architectureId);
        return !architectureId.isEmpty() && !singerId.isEmpty() && architectureIt != d->architectures.cend() &&
               architectureIt->singers.contains(singerId);
    }

    SingerInfo SingerRegistry::singerInfo(const QString &architectureId, const QString &singerId) const {
        Q_D(const SingerRegistry);
        const auto architectureIt = d->architectures.constFind(architectureId);
        if (architectureIt == d->architectures.cend())
            return {};
        const auto singerIt = architectureIt->singers.constFind(singerId);
        return singerIt == architectureIt->singers.cend() ? SingerInfo{} : *singerIt;
    }

    QStringList SingerRegistry::singerIds(const QString &architectureId) const {
        Q_D(const SingerRegistry);
        const auto architectureIt = d->architectures.constFind(architectureId);
        return architectureIt == d->architectures.cend() ? QStringList{} : architectureIt->singers.keys();
    }

}

#include "moc_SingerRegistry.cpp"
