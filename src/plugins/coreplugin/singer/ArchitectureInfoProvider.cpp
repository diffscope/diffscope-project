#include "ArchitectureInfoProvider.h"
#include "ArchitectureInfoProvider_p.h"

#include <utility>

#include <coreplugin/SingerRegistry.h>

namespace Core {

    void ArchitectureInfoProviderPrivate::update() {
        Q_Q(ArchitectureInfoProvider);
        const bool newExists = registry && registry->containsArchitecture(architectureId);
        const ArchitectureInfo newInfo = newExists ? registry->architectureInfo(architectureId) : ArchitectureInfo{};
        const QStringList newSingerIds = newExists ? registry->singerIds(architectureId) : QStringList{};
        if (info != newInfo) {
            info = newInfo;
            emit q->infoChanged(info);
        }
        if (exists != newExists) {
            exists = newExists;
            emit q->existsChanged(exists);
        }
        if (singerIds != newSingerIds) {
            singerIds = newSingerIds;
            emit q->singerIdsChanged(singerIds);
        }
    }

    void ArchitectureInfoProviderPrivate::disconnectRegistry() {
        for (const auto &connection : std::as_const(registryConnections))
            QObject::disconnect(connection);
        registryConnections.clear();
    }

    ArchitectureInfoProvider::ArchitectureInfoProvider(QObject *parent)
        : QObject(parent), d_ptr(new ArchitectureInfoProviderPrivate) {
        Q_D(ArchitectureInfoProvider);
        d->q_ptr = this;
    }

    ArchitectureInfoProvider::~ArchitectureInfoProvider() = default;

    SingerRegistry *ArchitectureInfoProvider::registry() const {
        Q_D(const ArchitectureInfoProvider);
        return d->registry;
    }

    void ArchitectureInfoProvider::setRegistry(SingerRegistry *registry) {
        Q_D(ArchitectureInfoProvider);
        if (d->registry == registry)
            return;

        d->disconnectRegistry();
        d->registry = registry;
        if (registry) {
            const auto updateIfMatching = [this](const QString &architectureId) {
                Q_D(ArchitectureInfoProvider);
                if (d->architectureId == architectureId)
                    d->update();
            };
            const auto updateSingerIdsIfMatching = [this](const QString &architectureId, const QString &) {
                Q_D(ArchitectureInfoProvider);
                if (d->architectureId == architectureId)
                    d->update();
            };
            d->registryConnections.append(connect(registry, &SingerRegistry::architectureRegistered, this,
                                                  updateIfMatching));
            d->registryConnections.append(connect(registry, &SingerRegistry::architectureUpdated, this,
                                                  updateIfMatching));
            d->registryConnections.append(connect(registry, &SingerRegistry::architectureRemoved, this,
                                                  updateIfMatching));
            d->registryConnections.append(connect(registry, &SingerRegistry::singerRegistered, this,
                                                  updateSingerIdsIfMatching));
            d->registryConnections.append(connect(registry, &SingerRegistry::singerRemoved, this,
                                                  updateSingerIdsIfMatching));
            d->registryConnections.append(connect(registry, &QObject::destroyed, this, [this] {
                Q_D(ArchitectureInfoProvider);
                d->disconnectRegistry();
                d->registry = nullptr;
                emit registryChanged(nullptr);
                d->update();
            }));
        }

        emit registryChanged(registry);
        d->update();
    }

    QString ArchitectureInfoProvider::architectureId() const {
        Q_D(const ArchitectureInfoProvider);
        return d->architectureId;
    }

    void ArchitectureInfoProvider::setArchitectureId(const QString &architectureId) {
        Q_D(ArchitectureInfoProvider);
        if (d->architectureId == architectureId)
            return;
        d->architectureId = architectureId;
        emit architectureIdChanged(d->architectureId);
        d->update();
    }

    ArchitectureInfo ArchitectureInfoProvider::info() const {
        Q_D(const ArchitectureInfoProvider);
        return d->info;
    }

    bool ArchitectureInfoProvider::exists() const {
        Q_D(const ArchitectureInfoProvider);
        return d->exists;
    }

    QStringList ArchitectureInfoProvider::singerIds() const {
        Q_D(const ArchitectureInfoProvider);
        return d->singerIds;
    }

}

#include "moc_ArchitectureInfoProvider.cpp"
