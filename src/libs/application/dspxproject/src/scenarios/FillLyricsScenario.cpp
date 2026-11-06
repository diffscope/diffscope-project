// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "FillLyricsScenario.h"
#include "FillLyricsScenario_p.h"

#include <algorithm>

#include <QLoggingCategory>
#include <QPointer>
#include <QQuickWindow>
#include <QSet>

#include <dspxmodelORM/Note.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxproject/Document.h>
#include <dspxproject/FillLyricsStrategy.h>
#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcFillLyricsScenario, "diffscope.dspxproject.filllyricsscenario")

    FillLyricsScenario::FillLyricsScenario(Document *document, FillLyricsStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new FillLyricsScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy}) {
    }

    FillLyricsScenario::~FillLyricsScenario() = default;

    void FillLyricsScenario::fillLyrics(QQuickWindow *window, DocumentEdit::Hints hints) const {
        Q_D(const FillLyricsScenario);
        const QPointer<Document> document(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        const auto selection = document->selectionModel();
        if (selection->selectionType() != dspx::SelectionModel::ST_Note) {
            return;
        }
        const auto noteSelection = selection->noteSelectionModel();
        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock) {
            qCWarning(lcFillLyricsScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }
        auto notes = noteSelection->selectedItems();
        if (notes.isEmpty()) {
            return;
        }
        std::ranges::sort(notes, {}, &dspx::Note::position);
        QStringList initialLyrics;
        initialLyrics.reserve(notes.size());
        for (const auto note : notes) {
            initialLyrics.append(note->lyric());
        }

        const auto result = d->strategy->requestLyrics(initialLyrics, notes.size() != 1, dialogWindow, hints);
        if (!result || !readLock || !document || result->lyrics.isEmpty()) {
            return;
        }
        const auto originalCount = notes.size();
        if (!result->truncateToSelection && originalCount < result->lyrics.size()) {
            const QSet<dspx::Note *> selectedNotes(notes.cbegin(), notes.cend());
            for (auto note = notes.constLast()->nextItem(); note && notes.size() < result->lyrics.size(); note = note->nextItem()) {
                if (!selectedNotes.contains(note)) {
                    notes.append(note);
                }
            }
        }

        QList<QPointer<dspx::Note>> targets;
        const auto count = qMin(notes.size(), result->lyrics.size());
        targets.reserve(count);
        for (qsizetype index = 0; index < count; ++index) {
            targets.append(notes.at(index));
        }
        readLock.release();
        if (!document) {
            return;
        }
        bool applied = false;
        document->transactionController()->beginScopedTransaction(tr("Filling lyrics"), [&] {
            if (std::ranges::any_of(targets, [](const auto &note) { return note.isNull(); })) {
                return false;
            }
            bool changed = false;
            for (qsizetype index = 0; index < count; ++index) {
                auto note = targets.at(index);
                const auto &lyric = result->lyrics.at(index);
                if (note->lyric() != lyric) {
                    note->setLyric(lyric);
                    changed = true;
                }
            }
            applied = true;
            return changed;
        }, [strategy = d->strategy] {
            qCWarning(lcFillLyricsScenario) << "The fill lyrics transaction could not be started.";
            strategy->reportOperationConflict();
        });
        if (applied && document) {
            for (qsizetype index = originalCount; index < count; ++index) {
                if (auto note = targets.at(index)) {
                    selection->select(note, dspx::SelectionModel::Select);
                }
            }
        }
    }

}

#include "moc_FillLyricsScenario.cpp"
