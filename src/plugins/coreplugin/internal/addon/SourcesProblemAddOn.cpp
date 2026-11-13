// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "SourcesProblemAddOn.h"

#include <QMetaObject>
#include <QQuickWindow>
#include <QStringList>
#include <QVariantMap>

#include <QAKCore/actionregistry.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SourcesPickerModel.h>

#include <dspxproject/Document.h>
#include <dspxproject/DocumentProblemManager.h>
#include <dspxproject/EditSourcesScenario.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/SourcesSelection.h>

#include <singercatalog/SingerRegistry.h>
#include <transactional/TransactionController.h>

namespace Core::Internal {

    using DspxProject::DocumentProblem;

    static const auto sourcesProblemType = QStringLiteral("org.diffscope.core.sources");

    SourcesProblemAddOn::SourcesProblemAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    SourcesProblemAddOn::~SourcesProblemAddOn() = default;

    void SourcesProblemAddOn::initialize() {
        m_document = windowHandle()->cast<ProjectWindowInterface>()->projectDocumentContext()->document();
    }

    void SourcesProblemAddOn::extensionsInitialized() {
        auto *window = windowHandle()->cast<ProjectWindowInterface>();
        auto *controller = m_document->transactionController();
        auto *registry = CoreInterface::singerRegistry();
        connect(registry, &SingerCatalog::SingerRegistry::architectureRegistered, this, &SourcesProblemAddOn::scheduleRefresh);
        connect(registry, &SingerCatalog::SingerRegistry::architectureRemoved, this, &SourcesProblemAddOn::scheduleRefresh);
        connect(registry, &SingerCatalog::SingerRegistry::singerRegistered, this, &SourcesProblemAddOn::scheduleRefresh);
        connect(registry, &SingerCatalog::SingerRegistry::singerUpdated, this, &SourcesProblemAddOn::scheduleRefresh);
        connect(registry, &SingerCatalog::SingerRegistry::singerRemoved, this, &SourcesProblemAddOn::scheduleRefresh);
        connect(controller, &TransactionController::transactionActiveChanged, this, [this](bool active) {
            if (!active && m_refreshPending)
                QMetaObject::invokeMethod(this, &SourcesProblemAddOn::refreshProblems, Qt::QueuedConnection);
        });

        const auto action = CoreInterface::actionRegistry()->actionInfo(QStringLiteral("org.diffscope.core.edit.editSingers"));
        m_document->problemManager()->registerAction(
            this, sourcesProblemType, action->text().toString(),
            [this, window](const DocumentProblem &problem) {
                return QMetaObject::invokeMethod(this, [this, window, id = problem.id] {
                    if (!m_document)
                        return;
                    const auto current = m_document->problemManager()->problem(id);
                    if (!current)
                        return;
                    auto *sources = qobject_cast<dspx::Sources *>(current->info.entity.data());
                    auto *clip = sources ? sources->singingClip() : nullptr;
                    if (!clip)
                        return;
                    SourcesPickerModel picker;
                    picker.fromSources(sources);
                    window->editSourcesScenario()->editSources(qobject_cast<QQuickWindow *>(window->window()), picker.sourcesSelection(), {clip});
                }, Qt::QueuedConnection);
            },
            [controller](const DocumentProblem &) {
                return controller->canBeginTransaction() && controller->canAcquireReadLock();
            }
        );
        scheduleRefresh();
    }

    void SourcesProblemAddOn::scheduleRefresh() {
        if (!m_document || m_refreshPending)
            return;
        m_refreshPending = true;
        if (!m_document->transactionController()->isTransactionActive())
            QMetaObject::invokeMethod(this, &SourcesProblemAddOn::refreshProblems, Qt::QueuedConnection);
    }

    void SourcesProblemAddOn::refreshProblems() {
        if (!m_document || !m_refreshPending || m_document->transactionController()->isTransactionActive())
            return;
        m_refreshPending = false;

        m_modelWatcher = std::make_unique<QObject>();
        const auto refresh = [this] { scheduleRefresh(); };
        auto *tracks = m_document->model()->tracks();
        connect(tracks, &dspx::TrackList::itemInserted, m_modelWatcher.get(), refresh);
        connect(tracks, &dspx::TrackList::itemRemoved, m_modelWatcher.get(), refresh);

        SourcesPickerModel picker;
        QList<DspxProject::DocumentProblemReport> reports;
        for (auto *track : tracks->items()) {
            auto *clips = track->clips();
            connect(clips, &dspx::ClipSequence::itemInserted, m_modelWatcher.get(), refresh);
            connect(clips, &dspx::ClipSequence::itemRemoved, m_modelWatcher.get(), refresh);
            for (auto *clip : clips->asRange()) {
                auto *singingClip = qobject_cast<dspx::SingingClip *>(clip);
                if (!singingClip)
                    continue;
                connect(singingClip, &dspx::SingingClip::sourcesChanged, m_modelWatcher.get(), refresh);
                auto *sources = singingClip->sources();
                if (!sources)
                    continue;
                connect(sources, &dspx::Sources::categoryChanged, m_modelWatcher.get(), refresh);
                watchSingerList(sources->singers());

                picker.fromSources(sources);
                if (picker.valid())
                    continue;

                QStringList reasons;
                if (picker.architectureId().isEmpty())
                    reasons.append(tr("No singer architecture is configured."));
                if (picker.empty())
                    reasons.append(tr("No singers are configured."));
                for (const auto &entry : picker.validationIssues()) {
                    const auto issue = entry.toMap();
                    const auto code = issue.value(QStringLiteral("code")).toString();
                    if (!picker.architectureExists() && (code == QStringLiteral("missingSinger") || code == QStringLiteral("rootMixGroupMismatch")))
                        continue;
                    reasons.append(issue.value(QStringLiteral("message")).toString());
                }
                reasons.removeDuplicates();
                reports.append({
                    QString::number(singingClip->handle().d),
                    {sourcesProblemType, DspxProject::DocumentProblemSeverity::Warning,
                     tr("The singer configuration for \"%1\" is invalid.\n%2").arg(singingClip->name(), reasons.join(u'\n')), sources, {}},
                });
                connect(singingClip, &dspx::Clip::nameChanged, m_modelWatcher.get(), refresh);
            }
        }
        m_document->problemManager()->replaceProblems(this, reports);
    }

    void SourcesProblemAddOn::watchSingerList(dspx::SingerList *singers) {
        const auto refresh = [this] { scheduleRefresh(); };
        connect(singers, &dspx::SingerList::itemsChanged, m_modelWatcher.get(), refresh);
        for (auto *singer : singers->items()) {
            if (auto *single = qobject_cast<dspx::SingleSinger *>(singer)) {
                connect(single, &dspx::SingleSinger::idChanged, m_modelWatcher.get(), refresh);
            } else if (auto *mixed = qobject_cast<dspx::MixedSinger *>(singer)) {
                connect(mixed, &dspx::MixedSinger::ratioChanged, m_modelWatcher.get(), refresh);
                watchSingerList(mixed->singers());
            }
        }
    }

}
