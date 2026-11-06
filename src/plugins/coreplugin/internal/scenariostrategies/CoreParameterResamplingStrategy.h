// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREPARAMETERRESAMPLINGSTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREPARAMETERRESAMPLINGSTRATEGY_H

#include <QCoreApplication>

#include <dspxproject/ParameterResamplingStrategy.h>

namespace Core::Internal {

    class CoreParameterResamplingStrategy : public DspxProject::ParameterResamplingStrategy {
        Q_DECLARE_TR_FUNCTIONS(Core::Internal::CoreParameterResamplingStrategy)
    public:
        using DspxProject::ParameterResamplingStrategy::ParameterResamplingStrategy;

        Confirmation confirmResampling(Operation operation, QQuickWindow *window) override;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREPARAMETERRESAMPLINGSTRATEGY_H
