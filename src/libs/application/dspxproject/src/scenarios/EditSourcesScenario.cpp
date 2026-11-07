// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditSourcesScenario.h"
#include "EditSourcesScenario_p.h"

#include <algorithm>
#include <numeric>
#include <vector>

#include <QLoggingCategory>
#include <QQuickWindow>

#include <opendspx/mixedsinger.h>
#include <opendspx/singlesinger.h>

#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>

#include <dspxproject/Document.h>
#include <dspxproject/EditSourcesStrategy.h>
#include <dspxproject/SourcesSelection.h>
#include <dspxproject/SingerComparison.h>
#include <singercatalog/SingerRegistry.h>

#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcEditSourcesScenario, "diffscope.dspxproject.editsourcesscenario")

    namespace {

        QList<double> logicalRatios(const QList<double> &storedRatios, int singerCount) {
            if (singerCount <= 0)
                return {};

            QList<double> result;
            result.reserve(singerCount);
            double sum = 0.0;
            for (int index = 0; index < singerCount - 1; ++index) {
                const double ratio = index < storedRatios.size() ? storedRatios.at(index) : 0.0;
                result.append(ratio);
                sum += ratio;
            }
            result.append(std::max(0.0, 1.0 - sum));
            return result;
        }

        QList<double> storedRatios(QList<double> logicalRatios) {
            if (logicalRatios.size() <= 1)
                return {};
            logicalRatios.removeLast();
            return logicalRatios;
        }

        QList<double> resizedRatios(const QList<double> &ratios, int oldSingerCount, int newSingerCount) {
            if (oldSingerCount == newSingerCount)
                return ratios;
            if (newSingerCount <= 0)
                return {};

            auto weights = logicalRatios(ratios, oldSingerCount);
            if (newSingerCount < oldSingerCount) {
                weights.resize(newSingerCount);
                const double sum = std::accumulate(weights.cbegin(), weights.cend(), 0.0);
                if (sum > 0.0) {
                    for (double &weight : weights)
                        weight /= sum;
                } else {
                    const double weight = 1.0 / static_cast<double>(newSingerCount);
                    std::fill(weights.begin(), weights.end(), weight);
                }
            } else {
                const double oldScale = static_cast<double>(oldSingerCount) / static_cast<double>(newSingerCount);
                for (double &weight : weights)
                    weight *= oldScale;
                const double newSingerRatio = 1.0 / static_cast<double>(newSingerCount);
                while (weights.size() < newSingerCount)
                    weights.append(newSingerRatio);
            }
            return storedRatios(std::move(weights));
        }

        std::vector<opendspx::SingerRef> toSingerVector(const QList<opendspx::SingerRef> &singers) {
            std::vector<opendspx::SingerRef> result;
            result.reserve(static_cast<std::size_t>(singers.size()));
            for (const auto &singer : singers)
                result.push_back(singer);
            return result;
        }

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

        void replaceSingers(dspx::Sources *sources, const std::vector<opendspx::SingerRef> &singers) {
            auto *singerList = sources->singers();
            const auto oldSingers = singerList->items();
            singerList->fromOpenDSPX(singers);
            for (auto *oldSinger : oldSingers)
                sources->model()->destroyItem(oldSinger);
        }

        bool updateSources(dspx::SingingClip *clip, const QString &architectureId,
                           const std::vector<opendspx::SingerRef> &singers) {
            if (architectureId.isEmpty() || singers.empty()) {
                if (auto *sources = clip->sources()) {
                    clip->model()->destroyItem(sources);
                    return true;
                }
                return false;
            }

            auto *sources = clip->sources();
            if (sources && sources->category() == architectureId &&
                std::ranges::equal(sources->singers()->toOpenDSPX(), singers, singerDefinitionEquals)) {
                return false;
            }
            if (!sources) {
                sources = clip->model()->createSources();
                clip->setSources(sources);
            }

            const int oldSingerCount = sources->singers()->size();
            const int newSingerCount = static_cast<int>(singers.size());
            if (oldSingerCount != newSingerCount) {
                for (auto *anchor : sources->dynamicMixingAnchors()->asRange())
                    anchor->setRatio(resizedRatios(anchor->ratio(), oldSingerCount, newSingerCount));
            }

            sources->setCategory(architectureId);
            replaceSingers(sources, singers);
            return true;
        }

    }

    void EditSourcesScenarioPrivate::applySources(const SourcesSelection &sources,
                                                   const QList<QPointer<dspx::SingingClip>> &clips, QQuickWindow *window) const {
        Q_Q(const EditSourcesScenario);
        const QPointer<EditSourcesStrategy> guardedStrategy(strategy);
        const QPointer<QQuickWindow> dialogWindow(window);
        const auto singers = toSingerVector(sources.singers);
        const auto languages = singerRegistry->singerInfo(sources.architectureId, firstSingerId(singers)).languages();
        const auto targetModel = document->model();
        bool hasUnsupportedLanguage = false;
        const bool started = document->transactionController()->beginScopedTransaction(q->tr("Editing sources"), [&] {
            bool changed = false;
            for (const auto &clip : clips) {
                if (!clip || clip->model() != targetModel || !updateSources(clip, sources.architectureId, singers)) {
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
