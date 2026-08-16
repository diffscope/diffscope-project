// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISTASK_P_H
#define DIFFSCOPE_SYNTH_SYNTHESISTASK_P_H

#include <synth/SynthesisTask.h>

namespace Synth {

    class SynthesisTaskPrivate {
        Q_DECLARE_PUBLIC(SynthesisTask)

    public:
        explicit SynthesisTaskPrivate(SynthesisTask *q) : q_ptr(q) {}

        static SynthesisTaskPrivate *get(SynthesisTask *task) { return task->d_func(); }

        SynthesisTask *q_ptr{};
        QUuid id{QUuid::createUuid()};
        SynthesisTaskRequest request;
        SynthesisTaskOptions options;
        SynthesisTaskResult result;
        SynthesisTask::State state{SynthesisTask::Queued};
        QUuid serviceId;
        QString serviceName;
        QDateTime createdAt{QDateTime::currentDateTimeUtc()};
        QDateTime startedAt;
        QDateTime finishedAt;
        QString errorMessage;
        QVariantList diagnostics;
        QString diagnosticFilePath;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISTASK_P_H
