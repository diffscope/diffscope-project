// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PianoRollAddOn.h"

#include <algorithm>
#include <cmath>

#include <QKeyEvent>
#include <QQmlComponent>
#include <QQuickItem>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/DockingContext.h>

#include <ScopicFlowCore/ClavierViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/TimeViewModel.h>

#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/internal/AdditionalTrackLoader.h>
#include <visualeditor/internal/EditorPreference.h>
#include <visualeditor/internal/ScrollAddOn.h>
#include <visualeditor/internal/SingingClipListModel.h>

namespace VisualEditor::Internal {

    static double boundedTimeViewStart(const dspx::SingingClip *clip, double desiredStart, double viewLength) {
        const double clipStart = clip->position();
        const double clipEnd = clipStart + clip->clipLength();
        const double latestStart = std::max(clipStart, clipEnd - viewLength);
        return std::clamp(desiredStart, clipStart, latestStart);
    }

    static double centeredClavierViewStart(double minKey, double maxKey, double viewKeyCount) {
        const double desiredStart = (minKey + maxKey + 1.0 + viewKeyCount) / 2.0;
        return std::clamp(desiredStart, std::min(viewKeyCount, 128.0), 128.0);
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
        bindEditingNavigation();
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
            if (!o) {
                qFatal() << component.errorString();
            }
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
            if (!o) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.visualeditor.panel.pianoRoll", o->property("pianoRollPanelComponent").value<QQmlComponent *>());
        }
    }

    void PianoRollAddOn::extensionsInitialized() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        connect(pianoRollPanelInterface(), &PianoRollPanelInterface::editingClipChanged, this, [this, window] {
            if (!pianoRollPanelInterface()->editingClip())
                return;

            if (auto context = window->getFirstObject<SVS::DockingContext>())
                context->show(context->panel("org.diffscope.visualeditor.panel.pianoRoll"));
        });
    }

    bool PianoRollAddOn::delayedInitialize() {
        QTimer::singleShot(0, this, [this] {
            auto panelInterface = pianoRollPanelInterface();
            resetTimeViewStart();
            auto clip = panelInterface->editingClip();
            auto firstNote = clip ? clip->notes()->firstItem() : nullptr;
            auto editArea = panelInterface->pianoRollView()->property("centerEditArea").value<QQuickItem *>();
            if (!firstNote || !editArea || editArea->width() <= 0 || editArea->height() <= 0)
                return;

            static constexpr int noteCount = 8;
            auto timeLayoutViewModel = panelInterface->timeLayoutViewModel();
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

            auto clavierViewModel = panelInterface->clavierViewModel();
            if (clavierViewModel->pixelDensity() > 0) {
                const double viewKeyCount = editArea->height() / clavierViewModel->pixelDensity();
                clavierViewModel->setStart(centeredClavierViewStart(keyNumber, keyNumber, viewKeyCount));
            }
        });
        return WindowInterfaceAddOn::delayedInitialize();
    }

    void PianoRollAddOn::bindEditingNavigation() {
        auto panelInterface = pianoRollPanelInterface();
        m_editingClipSelectorModel = static_cast<QSortFilterProxyModel *>(panelInterface->editingClipSelectorModel());
        auto sourceModel = static_cast<SingingClipListModel *>(m_editingClipSelectorModel->sourceModel());
        connect(panelInterface, &PianoRollPanelInterface::editingClipChanged, this, &PianoRollAddOn::updateClipNavigation);
        connect(sourceModel, &SingingClipListModel::clipSequenceChanged, this, &PianoRollAddOn::updateTrackNavigation);
        connect(sourceModel, &SingingClipListModel::clipSequenceChanged, this, &PianoRollAddOn::updateClipNavigation);
        connect(m_editingClipSelectorModel, &QAbstractItemModel::rowsInserted, this, &PianoRollAddOn::updateClipNavigation);
        connect(m_editingClipSelectorModel, &QAbstractItemModel::rowsRemoved, this, &PianoRollAddOn::updateClipNavigation);
        connect(m_editingClipSelectorModel, &QAbstractItemModel::modelReset, this, &PianoRollAddOn::updateClipNavigation);
        connect(m_editingClipSelectorModel, &QAbstractItemModel::layoutChanged, this, &PianoRollAddOn::updateClipNavigation);

        auto tracks = panelInterface->windowHandle()->projectDocumentContext()->document()->model()->tracks();
        auto bindTrack = [this](dspx::Track *track) {
            connect(track->clips(), &dspx::ClipSequence::singingClipCountChanged, this, &PianoRollAddOn::updateTrackNavigation);
        };
        for (auto track : tracks->items()) {
            bindTrack(track);
        }
        connect(tracks, &dspx::TrackList::itemInserted, this, [bindTrack](int, dspx::Track *track) {
            bindTrack(track);
        });
        connect(tracks, &dspx::TrackList::itemRemoved, this, [this](int, dspx::Track *track) {
            disconnect(track->clips(), &dspx::ClipSequence::singingClipCountChanged, this, &PianoRollAddOn::updateTrackNavigation);
        });
        connect(tracks, &dspx::TrackList::itemsChanged, this, &PianoRollAddOn::updateTrackNavigation);
        connect(tracks, &dspx::TrackList::itemsChanged, this, &PianoRollAddOn::updateClipNavigation);
        updateTrackNavigation();
        updateClipNavigation();
    }

    void PianoRollAddOn::updateTrackNavigation() {
        auto panelInterface = pianoRollPanelInterface();
        auto clip = panelInterface->editingClip();
        auto currentTrack = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
        dspx::Track *previousTrack = nullptr;
        dspx::Track *nextTrack = nullptr;
        if (currentTrack) {
            auto tracks = panelInterface->windowHandle()->projectDocumentContext()->document()->model()->tracks();
            bool currentTrackFound = false;
            for (auto track : tracks->items()) {
                if (track == currentTrack) {
                    currentTrackFound = true;
                    continue;
                }
                if (track->clips()->singingClipCount() == 0)
                    continue;
                if (currentTrackFound) {
                    nextTrack = track;
                    break;
                }
                previousTrack = track;
            }
            if (!currentTrackFound) {
                previousTrack = nullptr;
            }
        }
        if (m_previousEditingTrack == previousTrack && m_nextEditingTrack == nextTrack)
            return;
        m_previousEditingTrack = previousTrack;
        m_nextEditingTrack = nextTrack;
        Q_EMIT trackNavigationChanged();
    }

    void PianoRollAddOn::updateClipNavigation() {
        auto sourceModel = static_cast<SingingClipListModel *>(m_editingClipSelectorModel->sourceModel());
        auto sequence = sourceModel->clipSequence();
        auto sourceIndex = sequence && sequence->track()->trackList() ? sourceModel->indexForClip(pianoRollPanelInterface()->editingClip()) : QModelIndex{};
        m_editingClipIndex = m_editingClipSelectorModel->mapFromSource(sourceIndex).row();
        Q_EMIT clipNavigationChanged();
    }

    bool PianoRollAddOn::canEditPreviousTrack() const {
        return !m_previousEditingTrack.isNull();
    }

    bool PianoRollAddOn::canEditNextTrack() const {
        return !m_nextEditingTrack.isNull();
    }

    bool PianoRollAddOn::canEditPreviousClip() const {
        return m_editingClipIndex > 0;
    }

    bool PianoRollAddOn::canEditNextClip() const {
        return m_editingClipIndex >= 0 && m_editingClipIndex + 1 < m_editingClipSelectorModel->rowCount();
    }

    int PianoRollAddOn::editingClipIndex() const {
        return m_editingClipIndex;
    }

    void PianoRollAddOn::editAdjacentTrack(bool next) {
        editTrack(next ? m_nextEditingTrack.data() : m_previousEditingTrack.data());
    }

    void PianoRollAddOn::editAdjacentClip(bool next) {
        if (next ? !canEditNextClip() : !canEditPreviousClip())
            return;
        auto index = m_editingClipSelectorModel->index(m_editingClipIndex + (next ? 1 : -1), 0);
        editClip(qobject_cast<dspx::SingingClip *>(index.data(SingingClipListModel::ClipObjectRole).value<dspx::Clip *>()));
    }

    void PianoRollAddOn::editTrack(dspx::Track *track) {
        if (!track || !track->trackList() || track->clips()->singingClipCount() == 0)
            return;

        auto panelInterface = pianoRollPanelInterface();
        auto editArea = panelInterface->pianoRollView()->property("centerEditArea").value<QQuickItem *>();
        const double pixelDensity = panelInterface->timeLayoutViewModel()->pixelDensity();
        const double viewStart = panelInterface->timeViewModel()->start();
        const double viewLength = editArea && pixelDensity > 0 ? std::max(0.0, editArea->width()) / pixelDensity : 0.0;
        const double viewEnd = viewStart + viewLength;
        dspx::SingingClip *nearestClip = nullptr;
        double nearestGap = 0.0;
        for (auto clip : track->clips()->asRange()) {
            if (clip->kind() != dspx::Clip::Singing)
                continue;
            const double clipStart = clip->position();
            const double clipEnd = clipStart + clip->clipLength();
            const double gap = std::max(viewStart - clipEnd, clipStart - viewEnd);
            if (!nearestClip || gap < nearestGap) {
                nearestClip = static_cast<dspx::SingingClip *>(clip);
                nearestGap = gap;
            }
            if (gap < 0 || clipStart >= viewEnd)
                break;
        }
        if (nearestClip) {
            editClip(nearestClip);
        }
    }

    void PianoRollAddOn::editClip(dspx::SingingClip *clip) {
        if (!clip || !clip->clipSequence() || !clip->clipSequence()->track()->trackList())
            return;
        auto panelInterface = pianoRollPanelInterface();
        const bool clipChanged = clip != panelInterface->editingClip();
        panelInterface->setEditingClip(clip);
        auto selectionModel = panelInterface->windowHandle()->projectDocumentContext()->document()->selectionModel();
        selectionModel->select(nullptr, dspx::SelectionModel::Select, dspx::SelectionModel::ST_Note, clip->notes());
        if (clipChanged) {
            adjustViewForEditingClip();
        }
    }

    void PianoRollAddOn::resetTimeViewStart() {
        auto panelInterface = pianoRollPanelInterface();
        auto clip = panelInterface->editingClip();
        auto editArea = panelInterface->pianoRollView()->property("centerEditArea").value<QQuickItem *>();
        auto timeLayoutViewModel = panelInterface->timeLayoutViewModel();
        if (!clip || !editArea || editArea->width() <= 0 || timeLayoutViewModel->pixelDensity() <= 0)
            return;

        const double viewLength = editArea->width() / timeLayoutViewModel->pixelDensity();
        auto firstNote = clip->notes()->firstItem();
        const double desiredStart = firstNote ? clip->position() + firstNote->position() - clip->clipStart() - viewLength / 3.0 : clip->position();
        panelInterface->timeViewModel()->setStart(boundedTimeViewStart(clip, desiredStart, viewLength));
    }

    void PianoRollAddOn::adjustViewForEditingClip() {
        auto panelInterface = pianoRollPanelInterface();
        auto clip = panelInterface->editingClip();
        auto editArea = panelInterface->pianoRollView()->property("centerEditArea").value<QQuickItem *>();
        auto timeLayoutViewModel = panelInterface->timeLayoutViewModel();
        if (!clip || !editArea || editArea->width() <= 0 || timeLayoutViewModel->pixelDensity() <= 0)
            return;

        auto timeViewModel = panelInterface->timeViewModel();
        const double viewLength = editArea->width() / timeLayoutViewModel->pixelDensity();
        const double clipStart = clip->position();
        const double clipEnd = clipStart + clip->clipLength();
        if (std::max(timeViewModel->start(), clipStart) >= std::min(timeViewModel->start() + viewLength, clipEnd)) {
            resetTimeViewStart();
        }

        auto clavierViewModel = panelInterface->clavierViewModel();
        if (editArea->height() <= 0 || clavierViewModel->pixelDensity() <= 0)
            return;

        const double visibleStart = std::max(timeViewModel->start(), clipStart);
        const double visibleEnd = std::min(timeViewModel->start() + viewLength, clipEnd);
        if (visibleStart >= visibleEnd)
            return;

        const double offset = static_cast<double>(clip->clipStart()) - clipStart;
        const int first = static_cast<int>(std::floor(visibleStart + offset));
        const int last = static_cast<int>(std::ceil(visibleEnd + offset));
        const auto notes = clip->notes()->slice(first, last - first);
        int minKey = 128;
        int maxKey = -1;
        for (auto note : notes) {
            if (note->length() == 0)
                continue;
            minKey = std::min(minKey, note->keyNumber());
            maxKey = std::max(maxKey, note->keyNumber());
        }
        if (maxKey < minKey)
            return;

        const double viewKeyCount = editArea->height() / clavierViewModel->pixelDensity();
        const double currentStart = clavierViewModel->start();
        if (maxKey + 1.0 <= currentStart && minKey >= currentStart - viewKeyCount)
            return;
        clavierViewModel->setStart(centeredClavierViewStart(minKey, maxKey, viewKeyCount));
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
