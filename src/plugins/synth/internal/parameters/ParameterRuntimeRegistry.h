// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_PARAMETERRUNTIMEREGISTRY_H
#define DIFFSCOPE_SYNTH_INTERNAL_PARAMETERRUNTIMEREGISTRY_H

#include <memory>
#include <mutex>

#include <QByteArray>
#include <QHash>
#include <QString>

#include <coreplugin/ArchitectureInfo.h>

#include <synth/ParameterConfiguration.h>

namespace Synth::Internal {

    class ParameterRuntimeRegistry {
    public:
        static ParameterRuntimeRegistry &instance();

        ParameterRuntimeRegistry(const ParameterRuntimeRegistry &) = delete;
        ParameterRuntimeRegistry &operator=(const ParameterRuntimeRegistry &) = delete;

        bool parameterInfo(const ParameterConfiguration &configuration, Core::ParameterInfo *result,
                           QString *errorMessage = nullptr);
        void clear();

    private:
        struct Context;

        ParameterRuntimeRegistry() = default;
        ~ParameterRuntimeRegistry();

        std::shared_ptr<Context> context(const QByteArray &handle) const;
        static double toDisplayValue(const Core::ParameterInfo &self, double value);
        static double fromDisplayValue(const Core::ParameterInfo &self, double value);
        static QString toDisplayString(const Core::ParameterInfo &self, double value);

        mutable std::mutex m_mutex;
        QHash<QByteArray, std::shared_ptr<Context>> m_contexts;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_PARAMETERRUNTIMEREGISTRY_H
