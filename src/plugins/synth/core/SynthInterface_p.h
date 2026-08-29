// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHINTERFACE_P_H
#define DIFFSCOPE_SYNTH_SYNTHINTERFACE_P_H

#include <synth/SynthInterface.h>

#include <QHash>
#include <QMap>

namespace Synth {

    class SynthInterfacePrivate {
        Q_DECLARE_PUBLIC(SynthInterface)

    public:
        explicit SynthInterfacePrivate(SynthInterface *q) : q_ptr(q) {}

        SynthInterface *q_ptr{};
        QList<ServiceInstanceConfiguration> serviceInstances;
        QHash<QUuid, ServiceInstanceDetails> serviceDetails;
        QMap<QString, ParameterConfiguration> builtinParameters;
        SynthesisTaskManager *taskManager{};
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHINTERFACE_P_H
