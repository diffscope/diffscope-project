#ifndef DIFFSCOPE_SYNTH_INTERNAL_COREMETADATAREGISTRY_H
#define DIFFSCOPE_SYNTH_INTERNAL_COREMETADATAREGISTRY_H

#include <QHash>
#include <QObject>
#include <QSet>

#include <synth/ParameterConfiguration.h>
#include <synth/ServiceTypes.h>

namespace Synth::Internal {

    class CoreMetadataRegistry final : public QObject {
    public:
        explicit CoreMetadataRegistry(QObject *parent = nullptr);
        ~CoreMetadataRegistry();

        void reconcile(const QList<ServiceInstanceConfiguration> &serviceOrder,
                       const QList<ServiceInstanceDetails> &details,
                       const QList<ParameterConfiguration> &parameterConfigurations);
        void clear();

    private:
        QSet<QString> m_ownedArchitectures;
        QHash<QString, QSet<QString>> m_ownedSingers;
        bool m_registryMutation{};
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_COREMETADATAREGISTRY_H
