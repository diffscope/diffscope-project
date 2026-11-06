// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreParameterResamplingStrategy.h"

#include <memory>

#include <QDialogButtonBox>
#include <QQmlComponent>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

namespace Core::Internal {

    DspxProject::ParameterResamplingStrategy::Confirmation CoreParameterResamplingStrategy::confirmResampling(Operation operation, QQuickWindow *window) {
        const auto text = operation == Operation::Paste
            ? tr("Pasting at this position requires resampling the parameter curve. Linear interpolation may change its shape.\n\nContinue to paste?")
            : tr("Bouncing these clips requires resampling parameter curves. Linear interpolation may change their shape.\n\nContinue to bounce?");
        enum Choice {
            AlwaysResample = 1,
            ResampleOnce,
            DoNotResample,
        };
        QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QObject> object(component.createWithInitialProperties({
            {"text", tr("Resample Parameters")},
            {"informativeText", text},
            {"buttons", QVariantList {
                QVariantMap {{"id", ResampleOnce}, {"text", tr("Once")}, {"role", QDialogButtonBox::AcceptRole}},
                QVariantMap {{"id", AlwaysResample}, {"text", tr("Always")}, {"role", QDialogButtonBox::ActionRole}},
                QVariantMap {{"id", DoNotResample}, {"text", tr("No")}, {"role", QDialogButtonBox::RejectRole}},
            }},
            {"primaryButton", ResampleOnce},
            {"escapeButton", DoNotResample},
            {"icon", SVS::SVSCraft::Warning},
            {"transientParent", QVariant::fromValue(window)},
        }));
        if (!object) {
            qFatal() << component.errorString();
        }
        auto *dialog = qobject_cast<QQuickWindow *>(object.get());
        if (!dialog) {
            qFatal("MessageBoxDialog in SVSCraft.UIComponents is not QQuickWindow");
        }
        const int choice = SVS::MessageBox::customExec(dialog).toInt();
        if (choice == AlwaysResample) {
            return Confirmation::Always;
        }
        return choice == ResampleOnce ? Confirmation::Once : Confirmation::Reject;
    }

}
