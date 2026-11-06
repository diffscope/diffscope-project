// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSTRATEGY_H

#include <QObject>

#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class DSPXPROJECT_EXPORT ParameterResamplingStrategy : public QObject {
        Q_OBJECT
    public:
        explicit ParameterResamplingStrategy(QObject *parent = nullptr);
        ~ParameterResamplingStrategy() override;

        enum class Operation { Paste, Bounce };
        enum class Confirmation { Reject, Once, Always };

        virtual Confirmation confirmResampling(Operation operation, QQuickWindow *window) = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSTRATEGY_H
