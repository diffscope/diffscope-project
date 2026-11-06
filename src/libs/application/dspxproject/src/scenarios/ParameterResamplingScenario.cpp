// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterResamplingScenario.h"
#include "ParameterResamplingScenario_p.h"

#include <QPointer>

namespace DspxProject {

    ParameterResamplingScenario::ParameterResamplingScenario(ParameterResamplingStrategy *strategy, const Preferences &preferences, QObject *parent)
        : QObject(parent), d_ptr(new ParameterResamplingScenarioPrivate{.q_ptr = this, .strategy = strategy, .preferences = preferences}) {
    }

    ParameterResamplingScenario::~ParameterResamplingScenario() = default;

    bool ParameterResamplingScenario::confirmPaste(QQuickWindow *window, bool requiresResampling) const {
        Q_D(const ParameterResamplingScenario);
        return d->confirmResampling(ParameterResamplingStrategy::Operation::Paste, window, requiresResampling);
    }

    bool ParameterResamplingScenario::confirmBounce(QQuickWindow *window, bool requiresResampling) const {
        Q_D(const ParameterResamplingScenario);
        return d->confirmResampling(ParameterResamplingStrategy::Operation::Bounce, window, requiresResampling);
    }

    bool ParameterResamplingScenarioPrivate::confirmResampling(ParameterResamplingStrategy::Operation operation, QQuickWindow *window, bool requiresResampling) const {
        if (!requiresResampling || preferences.skipResamplingCheck()) {
            return true;
        }
        const QPointer<ParameterResamplingScenario> scenario(q_ptr);
        const QPointer<ParameterResamplingStrategy> guardedStrategy(strategy);
        const auto choice = guardedStrategy->confirmResampling(operation, window);
        if (scenario && guardedStrategy && choice == ParameterResamplingStrategy::Confirmation::Always) {
            preferences.skipFutureResamplingChecks();
        }
        return scenario && guardedStrategy && choice != ParameterResamplingStrategy::Confirmation::Reject;
    }

}

#include "moc_ParameterResamplingScenario.cpp"
