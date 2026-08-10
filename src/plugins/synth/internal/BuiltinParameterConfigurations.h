#ifndef DIFFSCOPE_SYNTH_INTERNAL_BUILTINPARAMETERCONFIGURATIONS_H
#define DIFFSCOPE_SYNTH_INTERNAL_BUILTINPARAMETERCONFIGURATIONS_H

#include <QObject>

#include <synth/ParameterConfiguration.h>

namespace Synth::Internal {

    class BuiltinParameterConfigurations final : public QObject {
        Q_OBJECT
    public:
        static QList<ParameterConfiguration> all();
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_BUILTINPARAMETERCONFIGURATIONS_H
