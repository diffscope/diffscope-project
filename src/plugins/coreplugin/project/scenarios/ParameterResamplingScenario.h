// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PARAMETERRESAMPLINGSCENARIO_H
#define DIFFSCOPE_COREPLUGIN_PARAMETERRESAMPLINGSCENARIO_H

#include <QString>

#include <coreplugin/DocumentEditScenario.h>

namespace Core {

    struct FreeParameterClipboardData;

    class CORE_EXPORT ParameterResamplingScenario : public DocumentEditScenario {
        Q_OBJECT
        QML_ELEMENT

    public:
        explicit ParameterResamplingScenario(QObject *parent = nullptr);
        ~ParameterResamplingScenario() override;

        bool confirmPaste(const FreeParameterClipboardData &data, int targetStart) const;
        Q_INVOKABLE bool confirmBounce() const;

    private:
        bool confirmResampling(const QString &text) const;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PARAMETERRESAMPLINGSCENARIO_H
