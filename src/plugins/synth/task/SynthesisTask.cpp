// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisTask.h"
#include "SynthesisTask_p.h"

#include <memory>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>

namespace Synth {

    std::shared_ptr<void> SynthesisTaskPrivate::retainResources(SynthesisTask *task) {
        auto d = get(task);
        Q_ASSERT(!d->resourcesReleased);
        ++d->resourceUsers;
        return std::shared_ptr<void>(task, [task = QPointer<SynthesisTask>(task)](void *) {
            if (!task)
                return;
            auto d = get(task);
            Q_ASSERT(d->resourceUsers > 0);
            if (--d->resourceUsers == 0)
                d->scheduleCollection();
        });
    }

    SynthesisTask::SynthesisTask(const SynthesisTaskRequest &request, const SynthesisTaskOptions &options, QObject *parent)
        : QObject(parent), d_ptr(new SynthesisTaskPrivate(this)) {
        Q_D(SynthesisTask);
        d->type = request.type;
        d->displayName = request.displayName;
        d->request = request;
        d->options = options;
    }

    SynthesisTask::~SynthesisTask() = default;

    QUuid SynthesisTask::id() const {
        Q_D(const SynthesisTask);
        return d->id;
    }
    SynthesisTaskType SynthesisTask::type() const {
        Q_D(const SynthesisTask);
        return d->type;
    }
    SynthesisTask::State SynthesisTask::state() const {
        Q_D(const SynthesisTask);
        return d->state;
    }
    bool SynthesisTask::isActive() const {
        const auto current = state();
        return current == AwaitingResponse || current == StreamingQueued || current == Synthesizing;
    }
    bool SynthesisTask::isQueued() const {
        const auto current = state();
        return current == Queued || current == StreamingQueued;
    }
    double SynthesisTask::progress() const {
        Q_D(const SynthesisTask);
        return d->progress;
    }
    QString SynthesisTask::displayName() const {
        Q_D(const SynthesisTask);
        return d->displayName;
    }
    QUuid SynthesisTask::serviceInstanceId() const {
        Q_D(const SynthesisTask);
        return d->serviceId;
    }
    QString SynthesisTask::serviceInstanceName() const {
        Q_D(const SynthesisTask);
        return d->serviceName;
    }
    int SynthesisTask::priority() const {
        Q_D(const SynthesisTask);
        return d->options.priority;
    }
    QDateTime SynthesisTask::createdAt() const {
        Q_D(const SynthesisTask);
        return d->createdAt;
    }
    QDateTime SynthesisTask::startedAt() const {
        Q_D(const SynthesisTask);
        return d->startedAt;
    }
    QDateTime SynthesisTask::finishedAt() const {
        Q_D(const SynthesisTask);
        return d->finishedAt;
    }
    QString SynthesisTask::errorMessage() const {
        Q_D(const SynthesisTask);
        return d->errorMessage;
    }
    QVariantList SynthesisTask::diagnostics() const {
        Q_D(const SynthesisTask);
        if (!d->diagnostics.isEmpty() || d->diagnosticFilePath.isEmpty())
            return d->diagnostics;
        QFile file(d->diagnosticFilePath);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return QJsonDocument::fromJson(file.readAll()).object().value(QStringLiteral("exchanges")).toArray().toVariantList();
    }
    QString SynthesisTask::diagnosticFilePath() const {
        Q_D(const SynthesisTask);
        return d->diagnosticFilePath;
    }
    SynthesisTaskRequest SynthesisTask::request() const {
        Q_D(const SynthesisTask);
        return d->request;
    }
    SynthesisTaskOptions SynthesisTask::options() const {
        Q_D(const SynthesisTask);
        return d->options;
    }
    SynthesisTaskResult SynthesisTask::result() const {
        Q_D(const SynthesisTask);
        return d->result;
    }

    bool SynthesisTask::isFinished() const {
        const auto current = state();
        return current == Succeeded || current == Failed || current == Canceled;
    }

}

#include "moc_SynthesisTask.cpp"
