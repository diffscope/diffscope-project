// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditSourcesScenario.h"
#include "EditSourcesScenario_p.h"

#include <algorithm>
#include <vector>

#include <QLoggingCategory>
#include <QQuickWindow>

#include <opendspx/mixedsinger.h>
#include <opendspx/singlesinger.h>

#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>

#include <dspxproject/Document.h>
#include <dspxproject/EditSourcesStrategy.h>
#include <dspxproject/SourcesSelection.h>
#include <singercatalog/SingerRegistry.h>

#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcEditSourcesScenario, "diffscope.dspxproject.editsourcesscenario")

    namespace {

        QString firstSingerId(const std::vector<opendspx::SingerRef> &singers) {
            for (const auto &singer : singers) {
                if (singer->type == opendspx::Singer::Type::Single) {
                    const auto &single = static_cast<const opendspx::SingleSinger &>(*singer);
                    if (!single.id.empty())
                        return QString::fromStdString(single.id);
                } else if (singer->type == opendspx::Singer::Type::Mixed) {
                    const auto &mixed = static_cast<const opendspx::MixedSinger &>(*singer);
                    const auto id = firstSingerId(mixed.singers);
                    if (!id.isEmpty())
                        return id;
                }
            }
            return {};
        }

        QList<QPointer<dspx::SingingClip>> guardedClips(const QList<dspx::SingingClip *> &clips) {
            QList<QPointer<dspx::SingingClip>> result;
            result.reserve(clips.size());
            for (auto *clip : clips)
                result.append(clip);
            return result;
        }

    }

    void EditSourcesScenarioPrivate::applySources(const SourcesSelection &sources,
                                                 const QList<QPointer<dspx::SingingClip>> &clips, QQuickWindow *window) const {
        Q_Q(const EditSourcesScenario);
        const QPointer<EditSourcesStrategy> guardedStrategy(strategy);
        const QPointer<QQuickWindow> dialogWindow(window);
        const std::vector<opendspx::SingerRef> singers(sources.singers.cbegin(), sources.singers.cend());
        const auto languages = singerRegistry->singerInfo(sources.architectureId, firstSingerId(singers)).languages();
        const auto targetModel = document->model();
        bool hasUnsupportedLanguage = false;
        const bool started = document->transactionController()->beginScopedTransaction(q->tr("Editing sources"), [&] {
            bool changed = false;
            for (const auto &clip : clips) {
                if (!clip || clip->model() != targetModel || !sources.applyTo(clip)) {
                    continue;
                }
                changed = true;
                if (!languages.isEmpty() && !hasUnsupportedLanguage) {
                    hasUnsupportedLanguage = std::ranges::any_of(clip->notes()->asRange(), [&languages](const dspx::Note *note) {
                        const auto language = note->language();
                        return !language.isEmpty() && !languages.contains(language);
                    });
                }
            }
            return changed;
        }, [&] {
            qCWarning(lcEditSourcesScenario) << "Failed to edit sources because the transaction could not start.";
            strategy->reportOperationConflict();
        });
        if (started && hasUnsupportedLanguage && guardedStrategy) {
            guardedStrategy->reportUnsupportedNoteLanguages(dialogWindow);
        }
    }

    EditSourcesScenario::EditSourcesScenario(Document *document, EditSourcesStrategy *strategy, const SingerCatalog::SingerRegistry *singerRegistry, QObject *parent)
        : QObject(parent), d_ptr(new EditSourcesScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy, .singerRegistry = singerRegistry}) {
    }

    EditSourcesScenario::~EditSourcesScenario() = default;

    void EditSourcesScenario::editSources(QQuickWindow *window, const SourcesSelection &initial, const QList<dspx::SingingClip *> &clips, DocumentEdit::Hints hints) {
        Q_D(EditSourcesScenario);
        if (clips.isEmpty()) {
            return;
        }

        const QPointer<Document> document(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock) {
            qCWarning(lcEditSourcesScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }
        const auto guardedClipList = guardedClips(clips);
        const auto result = d->strategy->requestSources(initial, dialogWindow, hints);
        if (!result || !readLock || !document) {
            return;
        }
        readLock.release();
        if (document) {
            d->applySources(*result, guardedClipList, dialogWindow);
        }
    }

    void EditSourcesScenario::applySources(QQuickWindow *window, const SourcesSelection &sources, const QList<dspx::SingingClip *> &clips) {
        Q_D(EditSourcesScenario);
        if (!clips.isEmpty()) {
            d->applySources(sources, guardedClips(clips), window);
        }
    }

}

#include "moc_EditSourcesScenario.cpp"
