#ifndef DIFFSCOPE_SYNTH_SYNTHINTERFACE_P_H
#define DIFFSCOPE_SYNTH_SYNTHINTERFACE_P_H

#include <QHash>
#include <QMap>

#include <synth/SynthInterface.h>

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
