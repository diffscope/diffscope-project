#include "VirtualSingerPropertyEditorHelper.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <vector>

#include <QEventLoop>
#include <QLoggingCategory>
#include <QPointer>
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
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <opendspx/singer.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SourcesPickerModel.h>

#include <transactional/TransactionController.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcVirtualSingerPropertyEditorHelper, "diffscope.core.virtualsingerpropertyeditorhelper")

    namespace {

        QList<dspx::SingingClip *> selectedSingingClips(DspxDocument *document) {
            QList<dspx::SingingClip *> result;
            if (!document || !document->selectionModel())
                return result;

            const auto selectedClips = document->selectionModel()->clipSelectionModel()->selectedItems();
            result.reserve(selectedClips.size());
            for (auto *clip : selectedClips) {
                if (auto *singingClip = qobject_cast<dspx::SingingClip *>(clip))
                    result.append(singingClip);
            }
            return result;
        }

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

        QList<double> storedRatios(const QList<double> &logicalRatios) {
            if (logicalRatios.size() <= 1)
                return {};
            auto result = logicalRatios;
            result.removeLast();
            return result;
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
                const double oldScale = static_cast<double>(oldSingerCount)
                                        / static_cast<double>(newSingerCount);
                for (double &weight : weights)
                    weight *= oldScale;

                const double newSingerRatio = 1.0 / static_cast<double>(newSingerCount);
                while (weights.size() < newSingerCount)
                    weights.append(newSingerRatio);
            }
            return storedRatios(weights);
        }

        std::vector<opendspx::SingerRef> toSingerVector(const QList<opendspx::SingerRef> &singers) {
            std::vector<opendspx::SingerRef> result;
            result.reserve(static_cast<std::size_t>(singers.size()));
            for (const auto &singer : singers)
                result.push_back(singer);
            return result;
        }

        QList<opendspx::SingerRef> toSingerList(const std::vector<opendspx::SingerRef> &singers) {
            QList<opendspx::SingerRef> result;
            result.reserve(static_cast<qsizetype>(singers.size()));
            for (const auto &singer : singers)
                result.append(singer);
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
            auto *sources = clip->sources();
            if (!sources) {
                sources = clip->model()->createSources();
                clip->setSources(sources);
            }

            const int oldSingerCount = sources->singers()->size();
            const int newSingerCount = static_cast<int>(singers.size());
            if (oldSingerCount != newSingerCount) {
                for (auto *anchor : sources->dynamicMixingAnchors()->asRange()) {
                    anchor->setRatio(resizedRatios(anchor->ratio(), oldSingerCount, newSingerCount));
                }
            }

            sources->setCategory(architectureId);
            replaceSingers(sources, singers);
        }

    }

    VirtualSingerPropertyEditorHelper::VirtualSingerPropertyEditorHelper(QObject *parent)
        : QObject(parent) {
    }

    VirtualSingerPropertyEditorHelper::~VirtualSingerPropertyEditorHelper() = default;

    void VirtualSingerPropertyEditorHelper::editVirtualSinger(ProjectWindowInterface *windowHandle) {
        if (!windowHandle || !windowHandle->window() || !windowHandle->projectDocumentContext())
            return;

        QPointer<DspxDocument> document = windowHandle->projectDocumentContext()->document();
        const auto initialClips = selectedSingingClips(document.data());
        if (initialClips.isEmpty())
            return;

        SourcesPickerModel sourcesModel;
        sourcesModel.setRegistry(CoreInterface::singerRegistry());
        if (auto *sources = initialClips.first()->sources()) {
            sourcesModel.setArchitectureId(sources->category());
            const auto singers = sources->singers()->toOpenDSPX();
            sourcesModel.setSingers(toSingerList(singers));
        }

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "SourcesPickerDialog");
        if (component.isError()) {
            qCCritical(lcVirtualSingerPropertyEditorHelper) << component.errorString();
            return;
        }

        std::unique_ptr<QQuickWindow> dialog(qobject_cast<QQuickWindow *>(component.createWithInitialProperties({
            {"sourcesModel", QVariant::fromValue(&sourcesModel)},
        })));
        if (!dialog) {
            qCCritical(lcVirtualSingerPropertyEditorHelper) << component.errorString();
            return;
        }

        dialog->setTransientParent(windowHandle->window());
        dialog->setModality(Qt::ApplicationModal);
        m_dialogAccepted = false;

        QEventLoop eventLoop;
        connect(dialog.get(), SIGNAL(accepted()), this, SLOT(handleDialogAccepted()));
        connect(dialog.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
        dialog->show();
        eventLoop.exec();

        if (!m_dialogAccepted || !document)
            return;

        const auto selectedClips = selectedSingingClips(document.data());
        if (selectedClips.isEmpty())
            return;

        const QString architectureId = sourcesModel.architectureId();
        const auto singers = toSingerVector(sourcesModel.singers());
        document->transactionController()->beginScopedTransaction(tr("Editing virtual singer"), [&] {
            for (auto *clip : selectedClips)
                updateSources(clip, architectureId, singers);
            return true;
        }, [] {
            qCCritical(lcVirtualSingerPropertyEditorHelper)
                << "Failed to edit virtual singer in exclusive transaction";
        });
    }

    void VirtualSingerPropertyEditorHelper::handleDialogAccepted() {
        m_dialogAccepted = true;
    }

}

#include "moc_VirtualSingerPropertyEditorHelper.cpp"
