// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisPieceTrackAddOn.h"

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <synth/ProjectSynthesisContext.h>

#include <synthvisualizer/internal/SynthesisPieceModel.h>

namespace SynthVisualizer::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcSynthesisPieceTrackAddOn, "diffscope.synthvisualizer.piecetrackaddon")

    SynthesisPieceTrackAddOn::SynthesisPieceTrackAddOn(QObject *parent)
        : WindowInterfaceAddOn(parent) {
    }

    SynthesisPieceTrackAddOn::~SynthesisPieceTrackAddOn() = default;

    QAbstractItemModel *SynthesisPieceTrackAddOn::pieceModel() const {
        return m_pieceModel;
    }

    void SynthesisPieceTrackAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface) {
            qCWarning(lcSynthesisPieceTrackAddOn) << "The window is not a project window";
            return;
        }

        auto synthesisContext = Synth::ProjectSynthesisContext::of(windowInterface);
        if (!synthesisContext) {
            qCWarning(lcSynthesisPieceTrackAddOn) << "The project synthesis context is unavailable";
        }
        m_pieceModel = new SynthesisPieceModel(synthesisContext, this);

        QQmlComponent component(
            Core::RuntimeInterface::qmlEngine(),
            QStringLiteral("DiffScope.SynthVisualizer"),
            QStringLiteral("SynthesisPieceTrack"),
            this
        );
        if (component.isError()) {
            qCWarning(lcSynthesisPieceTrackAddOn) << component.errorString();
            return;
        }
        auto object = component.createWithInitialProperties({
            {QStringLiteral("addOn"), QVariant::fromValue(this)},
        });
        if (!object) {
            qCWarning(lcSynthesisPieceTrackAddOn) << component.errorString();
            return;
        }
        object->setParent(this);

        auto trackComponent = object->property("synthesisPieceTrackComponent").value<QQmlComponent *>();
        if (!trackComponent) {
            qCWarning(lcSynthesisPieceTrackAddOn) << "The synthesis piece track component is unavailable";
            return;
        }
        windowInterface->actionContext()->addAction(
            QStringLiteral("org.diffscope.synthvisualizer.pianoRollPanel.additionalTracks.synthesisPieces"),
            trackComponent
        );
    }

    void SynthesisPieceTrackAddOn::extensionsInitialized() {
    }

    bool SynthesisPieceTrackAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}

#include "moc_SynthesisPieceTrackAddOn.cpp"
