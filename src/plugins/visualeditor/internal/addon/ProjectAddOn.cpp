// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectAddOn.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <QMetaObject>
#include <QPointer>
#include <QQuickItem>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftQuick/DockingContext.h>
#include <SVSCraftQuick/DockingPanel.h>
#include <ScopicFlowCore/ListViewModel.h>
#include <ScopicFlowCore/TrackViewModel.h>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TimeSignature.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <dspxproject/Document.h>
#include <dspxproject/DocumentProblemManager.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <singercatalog/ArchitectureInfo.h>

#include <visualeditor/ArrangementPanelInterface.h>
#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/ProjectViewModelContext.h>
#include <visualeditor/internal/ScrollAddOn.h>

namespace VisualEditor::Internal {
    ProjectAddOn::ProjectAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ProjectAddOn::~ProjectAddOn() = default;

    void ProjectAddOn::initialize() {
        auto *window = windowHandle()->cast<Core::ProjectWindowInterface>();
        new ProjectViewModelContext(window);
    }

    void ProjectAddOn::extensionsInitialized() {
        auto *window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto *manager = window->projectDocumentContext()->document()->problemManager();
        manager->registerNavigationHandler(this, [this](const auto &problem) { return navigateToProblem(problem); },
                                           [manager](const auto &problem) {
                                               const auto path = manager->entityPath(problem.id);
                                               for (auto *entity : path) {
                                                   if (dspx::SelectionModel::selectionTypeFromItem(entity) != dspx::SelectionModel::ST_None ||
                                                       qobject_cast<dspx::TimeSignature *>(entity))
                                                       return true;
                                               }
                                               return false;
                                           });
    }

    bool ProjectAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    bool ProjectAddOn::navigateToProblem(const DspxProject::DocumentProblem &problem) {
        clearPendingNavigation();
        auto *window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto *document = window->projectDocumentContext()->document();
        auto *scroll = window->getFirstObject<ScrollAddOn>();
        const auto path = document->problemManager()->entityPath(problem.id);
        auto *selection = document->selectionModel();
        dspx::EntityObject *selectionTarget = nullptr;
        dspx::SingingClip *singingClip = nullptr;
        dspx::Clip *clip = nullptr;
        dspx::Track *track = nullptr;
        dspx::Note *note = nullptr;
        dspx::Parameter *parameter = nullptr;
        auto *timeline = window->projectTimeline()->musicTimeline();
        std::optional<double> tick;
        for (auto *entity : path) {
            if (!selectionTarget && selection->selectionTypeFromItem(entity) != dspx::SelectionModel::ST_None)
                selectionTarget = entity;
            if (auto *item = qobject_cast<dspx::Clip *>(entity)) {
                clip = item;
                singingClip = qobject_cast<dspx::SingingClip *>(item);
            } else if (auto *item = qobject_cast<dspx::Track *>(entity)) {
                track = item;
            } else if (auto *item = qobject_cast<dspx::Note *>(entity)) {
                note = item;
            } else if (auto *item = qobject_cast<dspx::Parameter *>(entity)) {
                parameter = item;
            } else if (auto *item = qobject_cast<dspx::Tempo *>(entity)) {
                tick = item->position();
            } else if (auto *item = qobject_cast<dspx::Label *>(entity)) {
                tick = item->position();
            } else if (auto *item = qobject_cast<dspx::KeySignature *>(entity)) {
                tick = item->position();
            } else if (auto *item = qobject_cast<dspx::TimeSignature *>(entity)) {
                tick = timeline->create(item->index(), 0, 0).totalTick();
            }
        }

        auto *target = problem.info.entity.data();
        const auto *anchor = qobject_cast<dspx::AnchorNode *>(target);
        const auto *mixingAnchor = qobject_cast<dspx::DynamicMixingAnchor *>(target);
        const bool pianoRollTarget = singingClip && (note || anchor || mixingAnchor || parameter);
        auto *docking = window->getFirstObject<SVS::DockingContext>();
        if (pianoRollTarget) {
            auto *pianoRoll = PianoRollPanelInterface::of(window);
            pianoRoll->setEditingClip(singingClip);
            if (docking) {
                auto *panel = docking->panel("org.diffscope.visualeditor.panel.pianoRoll");
                if (panel && !panel->isShown())
                    docking->show(panel);
            }
        }
        if (selectionTarget)
            selection->select(selectionTarget, dspx::SelectionModel::ClearPreviousSelection |
                                                   dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem);

        if (pianoRollTarget) {
            double targetTick = clip->position();
            double tickLength = clip->clipLength();
            std::optional<double> key;
            double keyLength = 0;
            if (note) {
                targetTick = static_cast<double>(clip->start()) + note->position();
                tickLength = note->length();
                if (const auto *phoneme = qobject_cast<dspx::Phoneme *>(target)) {
                    const double noteStart = timeline->create(0, 0, qRound(targetTick)).millisecond();
                    const auto *nextPhoneme = phoneme->nextItem();
                    const double end = nextPhoneme ? timeline->create(noteStart + nextPhoneme->start()).totalTick() : targetTick + tickLength;
                    targetTick = timeline->create(noteStart + phoneme->start()).totalTick();
                    tickLength = std::max(0.0, end - targetTick);
                } else {
                    key = note->keyNumber();
                    keyLength = 1;
                }
            } else if (anchor) {
                targetTick = static_cast<double>(clip->start()) + anchor->x();
                tickLength = 0;
                if (singingClip->parameters()->item(QStringLiteral("pitch")) == parameter)
                    key = SingerCatalog::ParameterInfo::fromDspxModelValue(anchor->y()) * 128.0 + 0.5;
            } else if (mixingAnchor) {
                targetTick = static_cast<double>(clip->start()) + mixingAnchor->position();
                tickLength = 0;
            }
            auto *pianoRoll = PianoRollPanelInterface::of(window);
            auto *area = pianoRoll->pianoRollView()->property("centerEditArea").value<QQuickItem *>();
            const auto reveal = [this, area = QPointer(area), pianoRoll, scroll, clip = QPointer(singingClip), manager = QPointer(document->problemManager()),
                                 id = problem.id, targetTick, tickLength, key, keyLength] {
                if (!area || !clip || !manager || pianoRoll->editingClip() != clip) {
                    clearPendingNavigation();
                    return;
                }
                if (area->window() && area->isVisible() && area->width() > 0 && area->height() > 0) {
                    if (manager->problem(id))
                        scroll->revealInPianoRoll(targetTick, tickLength, key, keyLength);
                    clearPendingNavigation();
                }
            };
            m_pendingNavigation = {
                connect(area, &QQuickItem::windowChanged, this, reveal),
                connect(area, &QQuickItem::visibleChanged, this, reveal),
                connect(area, &QQuickItem::widthChanged, this, reveal),
                connect(area, &QQuickItem::heightChanged, this, reveal),
                connect(pianoRoll, &PianoRollPanelInterface::editingClipChanged, this, [this] { clearPendingNavigation(); }),
            };
            reveal();
            return true;
        }

        if (!clip && !track && tick && scroll->activeEditingArea() == ScrollAddOn::PianoRoll) {
            auto *pianoRoll = PianoRollPanelInterface::of(window);
            auto *area = pianoRoll->pianoRollView()->property("centerEditArea").value<QQuickItem *>();
            if (area && area->isVisible() && area->width() > 0 && area->height() > 0) {
                scroll->revealInPianoRoll(*tick, 0);
                return true;
            }
        }

        auto *arrangement = ArrangementPanelInterface::of(window);
        auto *area = arrangement->arrangementView()->property("centerEditArea").value<QQuickItem *>();
        if (!area || area->width() <= 0 || area->height() <= 0)
            return selectionTarget != nullptr;
        if (clip)
            tick = clip->position();
        int trackIndex = -1;
        if (track) {
            auto *context = ProjectViewModelContext::of(window);
            trackIndex = context->trackListViewModel()->items().indexOf(context->getTrackViewItemFromDocumentItem(track));
        }
        if (tick || trackIndex >= 0)
            scroll->revealInArrangement(tick, clip ? clip->clipLength() : 0, trackIndex);
        return selectionTarget || tick.has_value() || trackIndex >= 0;
    }

    void ProjectAddOn::clearPendingNavigation() {
        for (const auto &connection : std::exchange(m_pendingNavigation, QList<QMetaObject::Connection>{}))
            disconnect(connection);
    }
}

#include "moc_ProjectAddOn.cpp"
