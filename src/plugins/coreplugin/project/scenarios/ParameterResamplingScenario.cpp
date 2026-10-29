// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ParameterResamplingScenario.h"

#include <memory>

#include <QPointer>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/FreeParameterResampling.h>
#include <coreplugin/private/DspxDocument_p.h>

namespace Core {

    ParameterResamplingScenario::ParameterResamplingScenario(QObject *parent)
        : DocumentEditScenario(parent) {
    }

    ParameterResamplingScenario::~ParameterResamplingScenario() = default;

    bool ParameterResamplingScenario::confirmPaste(const FreeParameterClipboardData &data, int targetStart) const {
        if (Internal::BehaviorPreference::skipParameterResamplingCheck() ||
            !Internal::freeParameterRequiresResampling(data, targetStart)) {
            return true;
        }
        return confirmResampling(tr("Pasting at this position requires resampling the parameter curve. Linear interpolation may change its shape.\n\nContinue to paste?"));
    }

    bool ParameterResamplingScenario::confirmBounce() const {
        const QPointer<DspxDocument> target(document());
        if (!target)
            return false;
        if (!target->d_func()->canSafelyBounceIgnoringResampling()) {
            if (SVS::MessageBox::warning(
                    RuntimeInterface::qmlEngine(), window(), tr("Warning"),
                    tr("Some notes or curves cross clip boundaries, or data points overlap after merging. Bouncing may trim or overwrite this data.\n\nContinue to bounce?"),
                    SVS::SVSCraft::Yes | SVS::SVSCraft::No, SVS::SVSCraft::No
                ) != SVS::SVSCraft::Yes) {
                return false;
            }
        }
        if (!target || document() != target)
            return false;
        const bool confirmed = Internal::BehaviorPreference::skipParameterResamplingCheck() ||
            !target->d_func()->bounceRequiresResampling() ||
            confirmResampling(tr("Bouncing these clips requires resampling parameter curves. Linear interpolation may change their shape.\n\nContinue to bounce?"));
        return confirmed && target && document() == target;
    }

    bool ParameterResamplingScenario::confirmResampling(const QString &text) const {
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
                QVariantMap {{"id", ResampleOnce}, {"text", tr("Once")}},
                QVariantMap {{"id", AlwaysResample}, {"text", tr("Always")}},
                QVariantMap {{"id", DoNotResample}, {"text", tr("No")}},
            }},
            {"primaryButton", ResampleOnce},
            {"escapeButton", DoNotResample},
            {"icon", SVS::SVSCraft::Warning},
            {"transientParent", QVariant::fromValue(window())},
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
            Internal::BehaviorPreference::setSkipParameterResamplingCheck(true);
            Internal::BehaviorPreference::instance()->save();
            return true;
        }
        return choice == ResampleOnce;
    }

}

#include "moc_ParameterResamplingScenario.cpp"
