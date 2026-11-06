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

    class DSPXPROJECT_EXPORT ParameterResamplingScenario : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ParameterResamplingScenario)

    public:
        struct Preferences {
            std::function<bool()> skipResamplingCheck;
            std::function<void()> skipFutureResamplingChecks;
        };

        ParameterResamplingScenario(ParameterResamplingStrategy *strategy, const Preferences &preferences, QObject *parent = nullptr);
        ~ParameterResamplingScenario() override;

        bool confirmPaste(QQuickWindow *window, bool requiresResampling) const;
        bool confirmBounce(QQuickWindow *window, bool requiresResampling) const;

    private:
        QScopedPointer<ParameterResamplingScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSCENARIO_H
