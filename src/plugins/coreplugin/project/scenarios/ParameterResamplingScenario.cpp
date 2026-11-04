// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ParameterResamplingScenario.h"

#include <memory>

#include <QLoggingCategory>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/FreeParameterResampling.h>
#include <coreplugin/private/DocumentEditScenario_p.h>
#include <coreplugin/private/DspxDocument_p.h>
#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcParameterResamplingScenario, "diffscope.core.parameterresamplingscenario")

    ParameterResamplingScenario::ParameterResamplingScenario(QObject *parent)
        : DocumentEditScenario(parent) {
    }

    ParameterResamplingScenario::~ParameterResamplingScenario() = default;

    bool ParameterResamplingScenario::confirmPaste(const FreeParameterClipboardData &data, int targetStart) const {
        return confirmPaste(data, targetStart, nullptr);
    }

    bool ParameterResamplingScenario::confirmPaste(const FreeParameterClipboardData &data, int targetStart, ProjectWindowInterface *windowHandle) const {
        if (Internal::BehaviorPreference::skipParameterResamplingCheck() ||
            !Internal::freeParameterRequiresResampling(data, targetStart)) {
            return true;
        }
        auto *target = document();
        if (!target)
            return false;
        TransactionController::ScopedReadLock readLock(target->transactionController());
        if (!readLock) {
            qCWarning(lcParameterResamplingScenario) << "Could not acquire a document read lock.";
            if (windowHandle)
                windowHandle->sendOperationConflictNotification();
            return false;
        }
        return confirmResampling(tr("Pasting at this position requires resampling the parameter curve. Linear interpolation may change its shape.\n\nContinue to paste?"));
    }

    bool ParameterResamplingScenario::confirmBounce() const {
        const QPointer<DspxDocument> target(document());
        if (!target)
            return false;
        const bool warnAboutOverlap = !target->d_func()->canSafelyBounceIgnoringResampling();
        const bool warnAboutResampling = !Internal::BehaviorPreference::skipParameterResamplingCheck() && target->d_func()->bounceRequiresResampling();
        if (!warnAboutOverlap && !warnAboutResampling)
            return true;
        TransactionController::ScopedReadLock readLock(target->transactionController());
        if (!readLock) {
            qCWarning(lcParameterResamplingScenario) << "Could not acquire a document read lock.";
            DocumentEditScenarioPrivate::sendOperationConflictNotification(window());
            return false;
        }
        if (warnAboutOverlap) {
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
        const bool confirmed = !warnAboutResampling ||
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
