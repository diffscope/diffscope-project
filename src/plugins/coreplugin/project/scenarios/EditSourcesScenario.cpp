#include "EditSourcesScenario.h"
#include "EditSourcesScenario_p.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <vector>

#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/SourcesPickerModel.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcEditSourcesScenario, "diffscope.core.editsourcesscenario")

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

        void updateSources(dspx::SingingClip *clip, const QString &architectureId,
                           const std::vector<opendspx::SingerRef> &singers) {
            if (architectureId.isEmpty() || singers.empty()) {
                if (auto *sources = clip->sources())
                    clip->model()->destroyItem(sources);
                return;
            }

            auto *sources = clip->sources();
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
        }

    }

    bool EditSourcesScenarioPrivate::execDialog(SourcesPickerModel *model) {
        Q_Q(EditSourcesScenario);
        if (!window || !model)
            return false;

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "SourcesPickerDialog");
        if (component.isError()) {
            qCCritical(lcEditSourcesScenario) << component.errorString();
            return false;
        }

        std::unique_ptr<QQuickWindow> dialog(qobject_cast<QQuickWindow *>(component.createWithInitialProperties({
            {"sourcesModel", QVariant::fromValue(model)},
        })));
        if (!dialog) {
            qCCritical(lcEditSourcesScenario) << component.errorString();
            return false;
        }

        dialog->setTransientParent(window);
        dialog->setModality(Qt::ApplicationModal);
        dialogAccepted = false;
        QEventLoop eventLoop;
        QObject::connect(dialog.get(), SIGNAL(accepted()), q, SLOT(handleDialogAccepted()));
        QObject::connect(dialog.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
        dialog->show();
        eventLoop.exec();
        return dialogAccepted;
    }

    bool EditSourcesScenarioPrivate::applySourcesImpl(
        SourcesPickerModel *model, const QList<QPointer<dspx::SingingClip>> &clips) const {
        if (!model)
            return false;

        const QString architectureId = model->architectureId();
        const auto singers = toSingerVector(model->singers());
        bool applied = false;
        for (const auto &guardedClip : clips) {
            auto *clip = guardedClip.data();
            if (!clip)
                continue;
            updateSources(clip, architectureId, singers);
            applied = true;
        }
        return applied;
    }

    EditSourcesScenario::EditSourcesScenario(QObject *parent)
        : QObject(parent), d_ptr(new EditSourcesScenarioPrivate) {
        Q_D(EditSourcesScenario);
        d->q_ptr = this;
    }

    EditSourcesScenario::~EditSourcesScenario() = default;

    QQuickWindow *EditSourcesScenario::window() const {
        Q_D(const EditSourcesScenario);
        return d->window;
    }

    void EditSourcesScenario::setWindow(QQuickWindow *window) {
        Q_D(EditSourcesScenario);
        if (d->window == window)
            return;
        QObject::disconnect(d->windowDestroyedConnection);
        d->window = window;
        if (window) {
            d->windowDestroyedConnection = connect(window, &QObject::destroyed, this, [this] {
                Q_D(EditSourcesScenario);
                d->window = nullptr;
                emit windowChanged();
            });
        }
        emit windowChanged();
    }

    DspxDocument *EditSourcesScenario::document() const {
        Q_D(const EditSourcesScenario);
        return d->document;
    }

    void EditSourcesScenario::setDocument(DspxDocument *document) {
        Q_D(EditSourcesScenario);
        if (d->document == document)
            return;
        QObject::disconnect(d->documentDestroyedConnection);
        d->document = document;
        if (document) {
            d->documentDestroyedConnection = connect(document, &QObject::destroyed, this, [this] {
                Q_D(EditSourcesScenario);
                d->document = nullptr;
                emit documentChanged();
            });
        }
        emit documentChanged();
    }

    void EditSourcesScenario::editSources(SourcesPickerModel *initialModel,
                                           const QList<dspx::SingingClip *> &clips) {
        Q_D(EditSourcesScenario);
        if (!d->document || !d->window || !initialModel || clips.isEmpty())
            return;

        const QPointer<SourcesPickerModel> guardedModel(initialModel);
        const auto guardedClipList = guardedClips(clips);
        d->document->transactionController()->beginScopedTransaction(tr("Editing sources"), [d, guardedModel, guardedClipList] {
            return guardedModel
                   && d->execDialog(guardedModel.data())
                   && guardedModel
                   && d->applySourcesImpl(guardedModel.data(), guardedClipList);
        }, [] {
            qCCritical(lcEditSourcesScenario) << "Failed to edit sources in exclusive transaction";
        });
    }

    void EditSourcesScenario::applySources(SourcesPickerModel *model,
                                            const QList<dspx::SingingClip *> &clips) {
        Q_D(EditSourcesScenario);
        if (!d->document || !model || clips.isEmpty())
            return;

        const QPointer<SourcesPickerModel> guardedModel(model);
        const auto guardedClipList = guardedClips(clips);
        d->document->transactionController()->beginScopedTransaction(tr("Editing sources"), [d, guardedModel, guardedClipList] {
            return guardedModel && d->applySourcesImpl(guardedModel.data(), guardedClipList);
        }, [] {
            qCCritical(lcEditSourcesScenario) << "Failed to apply sources in exclusive transaction";
        });
    }

    void EditSourcesScenario::handleDialogAccepted() {
        Q_D(EditSourcesScenario);
        d->dialogAccepted = true;
    }

}

#include "moc_EditSourcesScenario.cpp"
