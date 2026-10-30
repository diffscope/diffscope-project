// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISTASK_P_H
#define DIFFSCOPE_SYNTH_SYNTHESISTASK_P_H

#include <synth/SynthesisTask.h>

#include <memory>

namespace Synth {

    class SynthesisTaskPrivate {
        Q_DECLARE_PUBLIC(SynthesisTask)

    public:
        explicit SynthesisTaskPrivate(SynthesisTask *q) : q_ptr(q) {}

        static SynthesisTaskPrivate *get(SynthesisTask *task) { return task->d_func(); }
        /// Retains execution data without changing QObject ownership.
        static std::shared_ptr<void> retainResources(SynthesisTask *task);

        void scheduleCollection();

        SynthesisTask *q_ptr{};
        SynthesisTaskManager *manager{};
        QUuid id{QUuid::createUuid()};
        SynthesisTaskType type{SynthesisTaskType::Pronunciation};
        QString displayName;
        SynthesisTaskRequest request;
        SynthesisTaskOptions options;
        SynthesisTaskResult result;
        SynthesisTask::State state{SynthesisTask::Queued};
        double progress{};
        QUuid serviceId;
        QString serviceName;
        QDateTime createdAt{QDateTime::currentDateTimeUtc()};
        QDateTime startedAt;
        QDateTime finishedAt;
        QString errorMessage;
        QVariantList diagnostics;
        QString diagnosticFilePath;
        int resourceUsers{};
        bool collectionScheduled{};
        bool resourcesReleased{};
        bool removalRequested{};
        bool deletionScheduled{};
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISTASK_P_H
