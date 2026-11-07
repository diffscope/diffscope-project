// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSCENARIO_H

#include <functional>

#include <QObject>
#include <QScopedPointer>

#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class ParameterResamplingScenarioPrivate;
    class ParameterResamplingStrategy;

    /** Coordinates free parameter resampling confirmation and its persistent preference. */
    class DSPXPROJECT_EXPORT ParameterResamplingScenario : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ParameterResamplingScenario)

    public:
        /** The required callbacks for the resampling confirmation preference. */
        struct Preferences {
            /** Returns whether confirmation is disabled. */
            std::function<bool()> skipResamplingCheck;
            /** Records acceptance of future resampling operations without confirmation. */
            std::function<void()> skipFutureResamplingChecks;
        };

        /**
         * Creates a scenario with a borrowed strategy and copied preference callbacks.
         * @pre strategy is non-null and valid when confirmation begins.
         * @pre Both callbacks are callable and their captures remain valid during use.
         */
        ParameterResamplingScenario(ParameterResamplingStrategy *strategy, const Preferences &preferences, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~ParameterResamplingScenario() override;

        /**
         * Returns whether pasting may proceed with an optional dialog window.
         * Returns true if resampling is unnecessary, checks are disabled, or the user
         * accepts. Returns false on rejection or dependency destruction during interaction.
         */
        bool confirmPaste(QQuickWindow *window, bool requiresResampling) const;
        /**
         * Returns whether bouncing may proceed with an optional dialog window.
         * Uses the same acceptance and cancellation rules as confirmPaste().
         */
        bool confirmBounce(QQuickWindow *window, bool requiresResampling) const;

    private:
        QScopedPointer<ParameterResamplingScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSCENARIO_H
