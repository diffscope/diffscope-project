// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisTask.h"

#include <synth/private/SynthesisTask_p.h>

namespace Synth {

    SynthesisTask::SynthesisTask(const SynthesisTaskRequest &request, const SynthesisTaskOptions &options, QObject *parent)
        : QObject(parent), d_ptr(new SynthesisTaskPrivate(this)) {
        Q_D(SynthesisTask);
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
        return d->request.type;
    }
    SynthesisTask::State SynthesisTask::state() const {
        Q_D(const SynthesisTask);
        return d->state;
    }
    QString SynthesisTask::displayName() const {
        Q_D(const SynthesisTask);
        return d->request.displayName;
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
        return d->diagnostics;
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
