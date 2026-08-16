// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PianoRollAddOn.h"

#include <algorithm>

#include <QKeyEvent>
#include <QQmlComponent>
#include <QQuickItem>
#include <QTimer>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <ScopicFlowCore/ClavierViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/TimeViewModel.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/internal/AdditionalTrackLoader.h>
#include <visualeditor/internal/EditorPreference.h>
#include <visualeditor/internal/ScrollAddOn.h>

namespace VisualEditor::Internal {

    static double boundedTimeViewStart(const dspx::SingingClip *clip, double desiredStart, double viewLength) {
        const double clipStart = clip->position();
        const double clipEnd = clipStart + clip->clipLength();
        const double latestStart = std::max(clipStart, clipEnd - viewLength);
        return std::clamp(desiredStart, clipStart, latestStart);
    }

    PianoRollAddOn::PianoRollAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    PianoRollAddOn::~PianoRollAddOn() = default;

    void PianoRollAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->window()->installEventFilter(this);
        m_additionalTrackLoader = new AdditionalTrackLoader("org.diffscope.visualeditor.pianoRollPanel.additionalTrackWidgets", this);
        m_bottomAdditionalTrackLoader = new AdditionalTrackLoader("org.diffscope.visualeditor.pianoRollPanel.bottomAdditionalTrackWidgets", this);
        auto pianoRollPanelInterface = new PianoRollPanelInterface(this, windowInterface);
        m_additionalTrackLoader->setContextObject(pianoRollPanelInterface);
        m_bottomAdditionalTrackLoader->setContextObject(pianoRollPanelInterface);

        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "PianoRollAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        }

        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "PianoRollPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
                {"scrollAddOn", QVariant::fromValue(windowInterface->getFirstObject<ScrollAddOn>())},
            });
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.pianoRoll", o->property("pianoRollPanelComponent").value<QQmlComponent *>());
        }
    }

    void PianoRollAddOn::extensionsInitialized() {
    }

    bool PianoRollAddOn::delayedInitialize() {
        QTimer::singleShot(0, this, [this] {
            auto panelInterface = pianoRollPanelInterface();
            auto clip = panelInterface->editingClip();
            auto firstNote = clip ? clip->notes()->firstItem() : nullptr;
            auto editArea = panelInterface->pianoRollView()->property("centerEditArea").value<QQuickItem *>();
            if (!firstNote || !editArea || editArea->width() <= 0 || editArea->height() <= 0)
                return;

            static constexpr int noteCount = 8;
            auto timeLayoutViewModel = panelInterface->timeLayoutViewModel();
            auto timeViewModel = panelInterface->timeViewModel();
            auto note = firstNote;
            auto i = 0;
            double keyNumber = 0;
            for (; note && i < noteCount; note = note->nextItem(), ++i) {
                if (timeLayoutViewModel->pixelDensity() > 0 && note->position() - firstNote->position() > editArea->width() / timeLayoutViewModel->pixelDensity()) {
                    break;
                }
                keyNumber += note->keyNumber();
            }
            keyNumber /= i;
            if (timeLayoutViewModel->pixelDensity() > 0) {
                const double viewLength = editArea->width() / timeLayoutViewModel->pixelDensity();
                const double firstNotePosition = clip->position() + firstNote->position() - clip->clipStart();
                timeViewModel->setStart(boundedTimeViewStart(clip, firstNotePosition - viewLength / 3.0, viewLength));
            }

            auto clavierViewModel = panelInterface->clavierViewModel();
            if (clavierViewModel->pixelDensity() > 0) {
                const double viewKeyCount = editArea->height() / clavierViewModel->pixelDensity();
                const double minimumStart = std::min(viewKeyCount, 128.0);
                clavierViewModel->setStart(std::clamp(keyNumber + 0.5 + viewKeyCount / 2.0, minimumStart, 128.0));
            }
        });
        return WindowInterfaceAddOn::delayedInitialize();
    }

    PianoRollPanelInterface *PianoRollAddOn::pianoRollPanelInterface() const {
        return PianoRollPanelInterface::of(windowHandle()->cast<Core::ProjectWindowInterface>());
    }

    AdditionalTrackLoader *PianoRollAddOn::additionalTrackLoader() const {
        return m_additionalTrackLoader;
    }

    AdditionalTrackLoader *PianoRollAddOn::bottomAdditionalTrackLoader() const {
        return m_bottomAdditionalTrackLoader;
    }

    bool PianoRollAddOn::eventFilter(QObject *watched, QEvent *event) {
        if (watched == windowHandle()->window()) {
            switch (event->type()) {
                case QEvent::KeyPress:
                case QEvent::KeyRelease: {
                    if (!EditorPreference::enableTemporarySnapOff()) {
                        break;
                    }
                    auto keyEvent = static_cast<QKeyEvent *>(event);
                    if (keyEvent->isAutoRepeat()) {
                        break;
                    }
                    if (keyEvent->key() == Qt::Key_Shift) {
                        pianoRollPanelInterface()->setSnapTemporarilyDisabled(keyEvent->type() == QEvent::KeyPress);
                    } else if (keyEvent->key() == Qt::Key_Alt) {
                        auto p = keyEvent->type() == QEvent::KeyPress;
                        if (m_altPressed != p) {
                            m_altPressed = p;
                            Q_EMIT altPressedChanged();
                        }
                    }
                    break;
                }
                case QEvent::FocusOut: {
                    pianoRollPanelInterface()->setSnapTemporarilyDisabled(false);
                    if (m_altPressed) {
                        m_altPressed = false;
                        Q_EMIT altPressedChanged();
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return WindowInterfaceAddOn::eventFilter(watched, event);
    }

    bool PianoRollAddOn::altPressed() const {
        return m_altPressed;
    }

    bool PianoRollAddOn::isTrackSelectorVisible() const {
        return m_trackSelectorVisible;
    }

    void PianoRollAddOn::setTrackSelectorVisible(bool visible) {
        if (m_trackSelectorVisible != visible) {
            m_trackSelectorVisible = visible;
            Q_EMIT trackSelectorVisibleChanged();
        }
    }

}

#include "moc_PianoRollAddOn.cpp"
