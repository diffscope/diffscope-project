// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSTRATEGY_H

#include <QObject>

#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    /** Supplies resampling confirmation. Window arguments are optional dialog parents. */
    class DSPXPROJECT_EXPORT ParameterResamplingStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit ParameterResamplingStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~ParameterResamplingStrategy() override;

        /** The operation requiring resampling. */
        enum class Operation {
            Paste, /**< Pasting a sampled parameter interval. */
            Bounce /**< Bouncing singing clips. */
        };
        /** The accepted scope of resampling permission. */
        enum class Confirmation {
            Reject, /**< Cancel the operation. */
            Once, /**< Accept this operation. */
            Always /**< Accept this operation and disable future confirmations. */
        };

        /** Returns the user's resampling decision for operation. */
        virtual Confirmation confirmResampling(Operation operation, QQuickWindow *window) = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PARAMETERRESAMPLINGSTRATEGY_H
