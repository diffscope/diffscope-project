#include "CoreMetadataRegistry.h"

#include <utility>

#include <QLoggingCategory>
#include <QMap>
#include <QScopedValueRollback>

#include <coreplugin/ArchitectureInfo.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/SingerInfo.h>
#include <coreplugin/SingerRegistry.h>

#include <synth/internal/ParameterRuntimeRegistry.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcCoreMetadataRegistry, "diffscope.synth.coremetadataregistry")

    namespace {

        struct DesiredArchitecture {
            QString name;
            QSet<QString> parameterIds;
        };

        struct DesiredMetadata {
            QMap<QString, DesiredArchitecture> architectures;
            QMap<QString, QMap<QString, SingerMetadata>> singers;
        };

        DesiredMetadata mergeMetadata(const QList<ServiceInstanceConfiguration> &serviceOrder,
                                      const QList<ServiceInstanceDetails> &details) {
            QHash<QUuid, ServiceInstanceDetails> detailsById;
            for (const auto &item : details)
                detailsById.insert(item.configuration().id(), item);

            DesiredMetadata result;
            for (const auto &service : serviceOrder) {
                if (!service.isEnabled())
                    continue;
                const auto detailsIt = detailsById.constFind(service.id());
                if (detailsIt == detailsById.cend())
                    continue;
                const auto metadata = detailsIt->metadata();
                for (const auto &architecture : metadata.architectures()) {
                    auto it = result.architectures.find(architecture.id());
                    if (it == result.architectures.end()) {
                        DesiredArchitecture desired;
                        desired.name = architecture.name();
                        it = result.architectures.insert(architecture.id(), desired);
                    }
                    for (const auto &parameter : architecture.parameters())
                        it->parameterIds.insert(parameter.id());
                }
                for (const auto &singer : metadata.singers()) {
                    if (!result.architectures.contains(singer.architectureId()))
                        continue;
                    auto &architectureSingers = result.singers[singer.architectureId()];
                    if (!architectureSingers.contains(singer.id()))
                        architectureSingers.insert(singer.id(), singer);
                }
            }
            return result;
        }

        Core::ArchitectureInfo architectureInfo(
            const QString &architectureId, const DesiredArchitecture &desired,
            const QHash<QString, ParameterConfiguration> &parameterConfigurations) {
            Core::ArchitectureInfo info;
            info.setName(desired.name);
            Core::ArchitectureInfo::ParameterMap parameters;
            for (const auto &parameterId : desired.parameterIds) {
                if (parameterId == QStringLiteral("pitch"))
                    continue;
                const auto configurationIt = parameterConfigurations.constFind(parameterId);
                if (configurationIt == parameterConfigurations.cend() ||
                    configurationIt->architectureId() != architectureId) {
                    continue;
                }
                Core::ParameterInfo parameterInfo;
                QString errorMessage;
                if (!ParameterRuntimeRegistry::instance().parameterInfo(
                        *configurationIt, &parameterInfo, &errorMessage)) {
                    qCWarning(lcCoreMetadataRegistry)
                        << "Could not create runtime metadata for parameter" << parameterId
                        << errorMessage;
                    continue;
                }
                parameters.insert(parameterId, parameterInfo);
            }
            info.setParameters(parameters);
            // TODO: Register an architecture-specific control panel when synthesis controls are
            // implemented. Per-service architecture schemas remain available through SynthInterface.
            return info;
        }

        Core::SingerInfo singerInfo(const SingerMetadata &source) {
            Core::SingerInfo info;
            info.setName(source.name());
            info.setAvatarUrl(source.avatarUrl());
            info.setBackgroundUrl(source.backgroundUrl());
            info.setDefaultLanguage(source.defaultLanguage());
            info.setLanguages(source.languages());
            info.setMixGroup(source.mixGroup());
            info.setDefaultExtra(source.defaultExtra());
            return info;
        }

    }

    CoreMetadataRegistry::CoreMetadataRegistry(QObject *parent)
        : QObject(parent) {
        auto registry = Core::CoreInterface::singerRegistry();
        if (!registry)
            return;
        connect(registry, &Core::SingerRegistry::architectureRemoved, this,
                [this](const QString &architectureId) {
            if (m_registryMutation)
                return;
            m_ownedArchitectures.remove(architectureId);
            m_ownedSingers.remove(architectureId);
        });
        connect(registry, &Core::SingerRegistry::architectureUpdated, this,
                [this](const QString &architectureId) {
            if (m_registryMutation)
                return;
            // Ownership is deliberately conservative: an entry changed by another component
            // must never subsequently be overwritten or removed by synth.
            m_ownedArchitectures.remove(architectureId);
            m_ownedSingers.remove(architectureId);
        });
        connect(registry, &Core::SingerRegistry::singerRemoved, this,
                [this](const QString &architectureId, const QString &singerId) {
            if (m_registryMutation)
                return;
            auto it = m_ownedSingers.find(architectureId);
            if (it == m_ownedSingers.end())
                return;
            it->remove(singerId);
            if (it->isEmpty())
                m_ownedSingers.erase(it);
        });
        connect(registry, &Core::SingerRegistry::singerUpdated, this,
                [this](const QString &architectureId, const QString &singerId) {
            if (m_registryMutation)
                return;
            auto it = m_ownedSingers.find(architectureId);
            if (it == m_ownedSingers.end())
                return;
            it->remove(singerId);
            if (it->isEmpty())
                m_ownedSingers.erase(it);
        });
    }

    CoreMetadataRegistry::~CoreMetadataRegistry() = default;

    void CoreMetadataRegistry::reconcile(
        const QList<ServiceInstanceConfiguration> &serviceOrder,
        const QList<ServiceInstanceDetails> &details,
        const QList<ParameterConfiguration> &parameterConfigurations) {
        auto registry = Core::CoreInterface::singerRegistry();
        if (!registry)
            return;
        const auto mutateRegistry = [this](auto &&operation) {
            QScopedValueRollback guard(m_registryMutation, true);
            return operation();
        };

        const auto desired = mergeMetadata(serviceOrder, details);
        QHash<QString, ParameterConfiguration> configurationsById;
        for (const auto &configuration : parameterConfigurations) {
            if (configuration.id() != QStringLiteral("pitch") &&
                !configurationsById.contains(configuration.id())) {
                configurationsById.insert(configuration.id(), configuration);
            }
        }

        // Remove singers first. SingerRegistry::removeArchitecture removes all singers, including
        // entries another plugin may have attached to an architecture originally owned by synth.
        for (auto architectureIt = m_ownedSingers.begin(); architectureIt != m_ownedSingers.end();) {
            const auto desiredSingers = desired.singers.value(architectureIt.key());
            auto &owned = architectureIt.value();
            for (auto singerIt = owned.begin(); singerIt != owned.end();) {
                if (!desired.architectures.contains(architectureIt.key()) ||
                    !desiredSingers.contains(*singerIt)) {
                    mutateRegistry([&] {
                        return registry->removeSinger(architectureIt.key(), *singerIt);
                    });
                    singerIt = owned.erase(singerIt);
                } else {
                    ++singerIt;
                }
            }
            if (owned.isEmpty())
                architectureIt = m_ownedSingers.erase(architectureIt);
            else
                ++architectureIt;
        }

        QSet<QString> blockedArchitectures;
        for (auto it = desired.architectures.cbegin(); it != desired.architectures.cend(); ++it) {
            const auto id = it.key();
            const auto info = architectureInfo(id, it.value(), configurationsById);
            if (m_ownedArchitectures.contains(id)) {
                if (!registry->containsArchitecture(id)) {
                    m_ownedArchitectures.remove(id);
                    m_ownedSingers.remove(id);
                } else {
                    mutateRegistry([&] { return registry->updateArchitecture(id, info); });
                    continue;
                }
            }
            if (registry->containsArchitecture(id)) {
                qCWarning(lcCoreMetadataRegistry)
                    << "Architecture id is already owned by another plugin; skipping" << id;
                blockedArchitectures.insert(id);
                continue;
            }
            if (mutateRegistry([&] { return registry->registerArchitecture(id, info); }))
                m_ownedArchitectures.insert(id);
        }

        for (auto architectureIt = desired.singers.cbegin();
             architectureIt != desired.singers.cend(); ++architectureIt) {
            const auto &architectureId = architectureIt.key();
            if (blockedArchitectures.contains(architectureId) ||
                !m_ownedArchitectures.contains(architectureId)) {
                continue;
            }
            for (auto singerIt = architectureIt->cbegin(); singerIt != architectureIt->cend(); ++singerIt) {
                const auto &singerId = singerIt.key();
                const auto info = singerInfo(singerIt.value());
                if (m_ownedSingers.value(architectureId).contains(singerId)) {
                    if (registry->containsSinger(architectureId, singerId))
                        mutateRegistry([&] {
                            return registry->updateSinger(architectureId, singerId, info);
                        });
                    else
                        m_ownedSingers[architectureId].remove(singerId);
                }
                if (!m_ownedSingers.value(architectureId).contains(singerId)) {
                    if (registry->containsSinger(architectureId, singerId)) {
                        qCWarning(lcCoreMetadataRegistry)
                            << "Singer id is already owned by another plugin; skipping"
                            << architectureId << singerId;
                        continue;
                    }
                    if (mutateRegistry([&] {
                            return registry->registerSinger(architectureId, singerId, info);
                        }))
                        m_ownedSingers[architectureId].insert(singerId);
                }
            }
        }

        for (auto it = m_ownedArchitectures.begin(); it != m_ownedArchitectures.end();) {
            if (desired.architectures.contains(*it)) {
                ++it;
                continue;
            }
            const auto id = *it;
            if (!registry->containsArchitecture(id)) {
                m_ownedSingers.remove(id);
                it = m_ownedArchitectures.erase(it);
                continue;
            }
            if (registry->singerIds(id).isEmpty() &&
                mutateRegistry([&] { return registry->removeArchitecture(id); })) {
                m_ownedSingers.remove(id);
                it = m_ownedArchitectures.erase(it);
            } else {
                auto neutralInfo = registry->architectureInfo(id);
                neutralInfo.setParameters(Core::ArchitectureInfo::ParameterMap{});
                mutateRegistry([&] { return registry->updateArchitecture(id, neutralInfo); });
                qCWarning(lcCoreMetadataRegistry)
                    << "Keeping an unused synth architecture because it contains foreign singers" << id;
                ++it;
            }
        }

        qsizetype singerCount{};
        for (const auto &singers : std::as_const(m_ownedSingers))
            singerCount += singers.size();
        qCInfo(lcCoreMetadataRegistry)
            << "Reconciled synthesis metadata with Core"
            << "architectures=" << m_ownedArchitectures.size()
            << "singers=" << singerCount
            << "blockedArchitectures=" << blockedArchitectures.size();
    }

    void CoreMetadataRegistry::clear() {
        auto registry = Core::CoreInterface::singerRegistry();
        if (!registry)
            return;
        const auto mutateRegistry = [this](auto &&operation) {
            QScopedValueRollback guard(m_registryMutation, true);
            return operation();
        };
        for (auto architectureIt = m_ownedSingers.begin(); architectureIt != m_ownedSingers.end();
             ++architectureIt) {
            for (const auto &singerId : std::as_const(architectureIt.value()))
                mutateRegistry([&] {
                    return registry->removeSinger(architectureIt.key(), singerId);
                });
        }
        m_ownedSingers.clear();
        for (const auto &architectureId : std::as_const(m_ownedArchitectures)) {
            if (!registry->containsArchitecture(architectureId))
                continue;
            if (registry->singerIds(architectureId).isEmpty())
                mutateRegistry([&] { return registry->removeArchitecture(architectureId); });
            else {
                auto neutralInfo = registry->architectureInfo(architectureId);
                neutralInfo.setParameters(Core::ArchitectureInfo::ParameterMap{});
                mutateRegistry([&] {
                    return registry->updateArchitecture(architectureId, neutralInfo);
                });
                qCWarning(lcCoreMetadataRegistry)
                    << "Could not remove synth architecture containing foreign singers"
                    << architectureId;
            }
        }
        m_ownedArchitectures.clear();
    }

}
