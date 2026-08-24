// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_BUILTINPARAMETERCONFIGURATIONS_H
#define DIFFSCOPE_SYNTH_INTERNAL_BUILTINPARAMETERCONFIGURATIONS_H

#include <QObject>

#include <synth/ParameterConfiguration.h>

namespace Synth::Internal {

    class BuiltinParameterConfigurations : public QObject {
        Q_OBJECT
    public:
        static QList<ParameterConfiguration> all();
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_BUILTINPARAMETERCONFIGURATIONS_H
