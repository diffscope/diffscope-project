// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_COREMETADATAREGISTRY_H
#define DIFFSCOPE_SYNTH_INTERNAL_COREMETADATAREGISTRY_H

#include <QHash>
#include <QObject>
#include <QSet>

#include <synth/ParameterConfiguration.h>
#include <synth/ServiceTypes.h>

namespace Synth::Internal {

    class CoreMetadataRegistry final : public QObject {
        Q_OBJECT

    public:
        explicit CoreMetadataRegistry(QObject *parent = nullptr);
        ~CoreMetadataRegistry() override;

        bool managesArchitecture(const QString &architectureId) const;

        void reconcile(const QList<ServiceInstanceConfiguration> &serviceOrder,
                       const QList<ServiceInstanceDetails> &details,
                       const QList<ParameterConfiguration> &parameterConfigurations);
        void clear();

    Q_SIGNALS:
        void managedArchitecturesChanged();

    private:
        // Registry ownership is retained for safe cleanup even while an architecture is not
        // currently offered by a synthesis service. Management is the narrower set that the
        // project scheduler is allowed to handle.
        QSet<QString> m_ownedArchitectures;
        QSet<QString> m_managedArchitectures;
        QHash<QString, QSet<QString>> m_ownedSingers;
        bool m_registryMutation{};
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_COREMETADATAREGISTRY_H
