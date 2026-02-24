#include "InsertItemScenario.h"
#include "InsertItemScenario_p.h"

#include <algorithm>
#include <utility>

#include <QCursor>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QVariant>
#include <QtGlobal>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/track.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/TrackColorSchema.h>

#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/TrackSelectionModel.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/BusControl.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSequence.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/private/DocumentEditScenario_p.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/DefaultLyricManager.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcInsertItemScenario, "diffscope.core.insertitemscenario")

    static void assignInitialTrackColor(int index, dspx::Track *item) {
        const auto colors = CoreInterface::trackColorSchema()->colors();
        if (colors.isEmpty()) {
            return;
        }

        const int n = colors.size();
        const int i = index % n;
        const int colorIndex = (i % 2) ? (n + i) / 2 : i / 2;
        item->setColorId(colorIndex);
    }

    InsertItemScenario::InsertItemScenario(QObject *parent)
        : DocumentEditScenario(parent), d_ptr(new InsertItemScenarioPrivate) {
        Q_D(InsertItemScenario);
        d->q_ptr = this;
    }

    InsertItemScenario::~InsertItemScenario() = default;

    ProjectTimeline *InsertItemScenario::projectTimeline() const {
        Q_D(const InsertItemScenario);
        return d->projectTimeline;
    }

    void InsertItemScenario::setProjectTimeline(ProjectTimeline *projectTimeline) {
        Q_D(InsertItemScenario);
        if (d->projectTimeline != projectTimeline) {
            d->projectTimeline = projectTimeline;
            Q_EMIT projectTimelineChanged();
        }
    }

    void InsertItemScenario::addTrack() const {
        Q_D(const InsertItemScenario);
        if (!document())
            return;
        qCInfo(lcInsertItemScenario) << "Adding track";
        auto model = document()->model();
        auto trackList = model->tracks();
        dspx::Track *newTrack = nullptr;
        bool success = false;
        auto selectionModel = document()->selectionModel();
        document()->transactionController()->beginScopedTransaction(tr("Adding track"), [=, &newTrack, &success] {
            newTrack = model->createTrack();
            newTrack->fromQDspx(QDspx::Track{});
            newTrack->setName(tr("Unnamed track"));
            auto insertionIndex = trackList->size();
            assignInitialTrackColor(insertionIndex, newTrack);
            do {
                auto currentTrack = qobject_cast<dspx::Track *>(selectionModel->currentItem());
                if (!currentTrack) {
                   break;
                }
                auto index = trackList->items().indexOf(currentTrack);
                if (index == -1) {
                   break;
                }
                insertionIndex = index + 1;
            } while (false);
            if (!trackList->insertItem(insertionIndex, newTrack)) {
                model->destroyItem(newTrack);
                newTrack = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Added track" << newTrack;
            return true;
        }, [] {
            qCCritical(lcInsertItemScenario) << "Failed to add track in exclusive transaction";
        });
        if (success && newTrack) {
            selectionModel->select(newTrack, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    void InsertItemScenario::insertTrack() const {
        Q_D(const InsertItemScenario);
        if (!document() || !window())
            return;
        qCInfo(lcInsertItemScenario) << "Inserting track";
        auto model = document()->model();
        auto trackList = model->tracks();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "InsertTrackDialog");
        QVariantMap properties;
        properties.insert("trackCount", trackList->size());
        properties.insert("insertionIndex", trackList->size());
        properties.insert("insertionCount", 1);
        properties.insert("trackName", tr("Unnamed track"));
        auto dialog = createAndPositionDialog(&component, properties);
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
            return;
        auto insertionIndex = dialog->property("insertionIndex").toInt();
        auto insertionCount = dialog->property("insertionCount").toInt();
        auto trackName = dialog->property("trackName").toString();
        qCDebug(lcInsertItemScenario) << "Inserting track at" << insertionIndex << "with" << insertionCount << "tracks" << "named" << trackName;
        insertionIndex = std::clamp(insertionIndex, 0, trackList->size());
        QList<dspx::Track *> newTracks;
        newTracks.reserve(insertionCount);
        bool success = false;
        document()->transactionController()->beginScopedTransaction(tr("Inserting track"), [=, &newTracks, &success] {
            for (int i = 0; i < insertionCount; ++i) {
                auto track = model->createTrack();
                track->fromQDspx(QDspx::Track{});
                track->setName(i == 0 ? trackName : QString("%1 (%L2)").arg(trackName).arg(i + 1));
                assignInitialTrackColor(insertionIndex + i, track);
                if (!trackList->insertItem(insertionIndex + i, track)) {
                    model->destroyItem(track);
                    return false;
                }
                newTracks.append(track);
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted tracks" << newTracks;
            return true;
        }, [] {
            qCCritical(lcInsertItemScenario) << "Failed to insert track in exclusive transaction";
        });
        if (success && !newTracks.isEmpty()) {
            auto selectionModel = document()->selectionModel();
            bool first = true;
            for (auto track : std::as_const(newTracks)) {
                auto command = dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem;
                if (first) {
                    command |= dspx::SelectionModel::ClearPreviousSelection;
                    first = false;
                }
                selectionModel->select(track, command);
            }
        }
    }

    void InsertItemScenario::insertLabel() const {
        Q_D(const InsertItemScenario);
        if (!document() || !d->projectTimeline || !window())
            return;
        qCInfo(lcInsertItemScenario) << "Inserting label";
        auto model = document()->model();
        auto labelSequence = model->timeline()->labels();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "InsertLabelDialog");
        QVariantMap properties;
        properties.insert("timeline", QVariant::fromValue(d->projectTimeline->musicTimeline()));
        properties.insert("labelPos", d->projectTimeline->position());
        properties.insert("labelText", QString());
        auto dialog = createAndPositionDialog(&component, properties);
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
            return;
        auto labelPos = dialog->property("labelPos").toInt();
        auto labelText = dialog->property("labelText").toString();
        qCDebug(lcInsertItemScenario) << "Inserting label at" << labelPos << "with text" << labelText;
        dspx::Label *newLabel = nullptr;
        bool success = false;
        document()->transactionController()->beginScopedTransaction(tr("Inserting label"), [=, &newLabel, &success] {
            newLabel = model->createLabel();
            newLabel->setPos(qMax(0, labelPos));
            newLabel->setText(labelText);
            if (!labelSequence->insertItem(newLabel)) {
                model->destroyItem(newLabel);
                newLabel = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted label" << newLabel;
            return true;
        }, [] {
            qCCritical(lcInsertItemScenario) << "Failed to insert label in exclusive transaction";
        });
        if (success && newLabel) {
            auto selectionModel = document()->selectionModel();
            selectionModel->select(newLabel, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    void InsertItemScenario::insertSingingClip() const {
        Q_D(const InsertItemScenario);
        if (!document() || !d->projectTimeline || !window())
            return;

        auto model = document()->model();
        auto trackList = model->tracks();
        if (!trackList || trackList->size() == 0)
            return;

        qCInfo(lcInsertItemScenario) << "Inserting singing clip";

        auto selectionModel = document()->selectionModel();
        auto trackSelectionModel = selectionModel->trackSelectionModel();
        auto currentTrack = trackSelectionModel ? trackSelectionModel->currentItem() : nullptr;
        if (!currentTrack) {
            if (auto currentClip = qobject_cast<dspx::Clip *>(selectionModel ? selectionModel->currentItem() : nullptr)) {
                if (auto clipSequence = currentClip->clipSequence()) {
                    currentTrack = clipSequence->track();
                }
            }
        }

        if (!currentTrack) {
            const auto tracks = trackList->items();
            if (!tracks.isEmpty()) {
                currentTrack = tracks.first();
            }
        }

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "InsertSingingClipDialog");
        QVariantMap properties;
        properties.insert("trackList", QVariant::fromValue(trackList));
        properties.insert("selectedTrack", QVariant::fromValue(currentTrack));
        properties.insert("timeline", QVariant::fromValue(d->projectTimeline->musicTimeline()));
        properties.insert("clipPosition", d->projectTimeline->position());
        properties.insert("clipLength", 46080);
        properties.insert("clipName", tr("Unnamed clip"));
        auto dialog = createAndPositionDialog(&component, properties);
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
            return;

        auto selectedTrack = qobject_cast<dspx::Track *>(dialog->property("selectedTrack").value<QObject *>());
        if (!selectedTrack) {
            selectedTrack = currentTrack;
        }
        if (!selectedTrack) {
            const auto tracks = trackList->items();
            if (!tracks.isEmpty()) {
                selectedTrack = tracks.first();
            }
        }

        if (!selectedTrack)
            return;

        const auto clipPosition = qMax(0, dialog->property("clipPosition").toInt());
        const auto clipLength = qMax(1, dialog->property("clipLength").toInt());
        const auto clipName = dialog->property("clipName").toString();
        qCDebug(lcInsertItemScenario) << "Inserting singing clip at" << clipPosition << "with length" << clipLength << "and name" << clipName << "to track" << selectedTrack;

        dspx::SingingClip *newClip = nullptr;
        bool success = false;
        document()->transactionController()->beginScopedTransaction(tr("Inserting singing clip"), [=, &newClip, &success] {
            newClip = model->createSingingClip();
            newClip->setName(clipName);
            auto time = newClip->time();
            time->setClipStart(0);
            time->setClipLen(clipLength);
            time->setStart(clipPosition);
            newClip->control()->setGain(1);
            auto clipSequence = selectedTrack->clips();
            if (!clipSequence || !clipSequence->insertItem(newClip)) {
                model->destroyItem(newClip);
                newClip = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted singing clip" << newClip;
            return true;
        }, [] {
            qCCritical(lcInsertItemScenario) << "Failed to insert singing clip in exclusive transaction";
        });

        if (success && newClip) {
            selectionModel->select(newClip, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    void InsertItemScenario::insertNote() const {
        Q_D(const InsertItemScenario);
        if (!document() || !d->projectTimeline || !window())
            return;

        auto model = document()->model();
        auto selectionModel = document()->selectionModel();
        auto noteSelectionModel = selectionModel->noteSelectionModel();

        auto noteSequence = noteSelectionModel->noteSequenceWithSelectedItems();
        if (!noteSequence)
            return;

        qCInfo(lcInsertItemScenario) << "Inserting note";

        auto clip = noteSequence->singingClip();

        // Calculate initial position: playback position - clip position
        const int clipPosition = clip->position();
        const int initialPosition = qMax(0, d->projectTimeline->position() - clipPosition);

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "InsertNoteDialog");
        QVariantMap properties;
        properties.insert("timeline", QVariant::fromValue(d->projectTimeline->musicTimeline()));
        properties.insert("notePosition", initialPosition);
        properties.insert("noteLength", 480);
        properties.insert("notePitch", 60); // Default to middle C (C4)
        properties.insert("noteLyric", CoreInterface::defaultLyricManager()->getDefaultLyricForSingingClip(clip));
        auto dialog = createAndPositionDialog(&component, properties);
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
            return;

        const auto notePosition = qMax(0, dialog->property("notePosition").toInt());
        const auto noteLength = qMax(1, dialog->property("noteLength").toInt());
        const auto notePitch = qBound(0, dialog->property("notePitch").toInt(), 127);
        const auto noteLyric = dialog->property("noteLyric").toString();
        qCDebug(lcInsertItemScenario) << "Inserting note at" << notePosition << "with length" << noteLength << "and pitch" << notePitch << "and lyric" << noteLyric << "to clip" << clip;

        dspx::Note *newNote = nullptr;
        bool success = false;
        document()->transactionController()->beginScopedTransaction(tr("Inserting note"), [=, &newNote, &success] {
            newNote = model->createNote();
            newNote->setPos(notePosition);
            newNote->setLength(noteLength);
            newNote->setKeyNum(notePitch);
            newNote->setLyric(noteLyric);
            if (!noteSequence->insertItem(newNote)) {
                model->destroyItem(newNote);
                newNote = nullptr;
                return false;
            }
            success = true;
            qCDebug(lcInsertItemScenario) << "Inserted note" << newNote;
            return true;
        }, [] {
            qCCritical(lcInsertItemScenario) << "Failed to insert note in exclusive transaction";
        });

        if (success && newNote) {
            selectionModel->select(newNote, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

}

#include "moc_InsertItemScenario.cpp"