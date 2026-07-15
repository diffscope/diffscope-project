#include "SingerInfoProvider.h"
#include "SingerInfoProvider_p.h"

#include <utility>

#include <coreplugin/SingerRegistry.h>

namespace Core {

    void SingerInfoProviderPrivate::update() {
        Q_Q(SingerInfoProvider);
        const bool newExists = registry && registry->containsSinger(architectureId, singerId);
        const SingerInfo newInfo = newExists ? registry->singerInfo(architectureId, singerId) : SingerInfo{};
        if (info != newInfo) {
            info = newInfo;
            emit q->infoChanged(info);
        }
        if (exists != newExists) {
            exists = newExists;
            emit q->existsChanged(exists);
        }
    }

    void SingerInfoProviderPrivate::disconnectRegistry() {
        for (const auto &connection : std::as_const(registryConnections))
            QObject::disconnect(connection);
        registryConnections.clear();
    }

    SingerInfoProvider::SingerInfoProvider(QObject *parent) : QObject(parent), d_ptr(new SingerInfoProviderPrivate) {
        Q_D(SingerInfoProvider);
        d->q_ptr = this;
    }

    SingerInfoProvider::~SingerInfoProvider() = default;

    SingerRegistry *SingerInfoProvider::registry() const {
        Q_D(const SingerInfoProvider);
        return d->registry;
    }

    void SingerInfoProvider::setRegistry(SingerRegistry *registry) {
        Q_D(SingerInfoProvider);
        if (d->registry == registry)
            return;

        d->disconnectRegistry();
        d->registry = registry;
        if (registry) {
            const auto updateIfMatching = [this](const QString &architectureId, const QString &singerId) {
                Q_D(SingerInfoProvider);
                if (d->architectureId == architectureId && d->singerId == singerId)
                    d->update();
            };
            d->registryConnections.append(connect(registry, &SingerRegistry::singerRegistered, this,
                                                  updateIfMatching));
            d->registryConnections.append(connect(registry, &SingerRegistry::singerUpdated, this,
                                                  updateIfMatching));
            d->registryConnections.append(connect(registry, &SingerRegistry::singerRemoved, this,
                                                  updateIfMatching));
            d->registryConnections.append(connect(registry, &QObject::destroyed, this, [this] {
                Q_D(SingerInfoProvider);
                d->disconnectRegistry();
                d->registry = nullptr;
                emit registryChanged(nullptr);
                d->update();
            }));
        }

        emit registryChanged(registry);
        d->update();
    }

    QString SingerInfoProvider::architectureId() const {
        Q_D(const SingerInfoProvider);
        return d->architectureId;
    }

    void SingerInfoProvider::setArchitectureId(const QString &architectureId) {
        Q_D(SingerInfoProvider);
        if (d->architectureId == architectureId)
            return;
        d->architectureId = architectureId;
        emit architectureIdChanged(d->architectureId);
        d->update();
    }

    QString SingerInfoProvider::singerId() const {
        Q_D(const SingerInfoProvider);
        return d->singerId;
    }

    void SingerInfoProvider::setSingerId(const QString &singerId) {
        Q_D(SingerInfoProvider);
        if (d->singerId == singerId)
            return;
        d->singerId = singerId;
        emit singerIdChanged(d->singerId);
        d->update();
    }

    SingerInfo SingerInfoProvider::info() const {
        Q_D(const SingerInfoProvider);
        return d->info;
    }

    bool SingerInfoProvider::exists() const {
        Q_D(const SingerInfoProvider);
        return d->exists;
    }

}

#include "moc_SingerInfoProvider.cpp"
