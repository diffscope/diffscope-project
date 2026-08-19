// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISTASKMANAGER_H
#define DIFFSCOPE_SYNTH_SYNTHESISTASKMANAGER_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QUuid>

#include <synth/SynthesisModel.h>
#include <synth/synthglobal.h>

namespace Synth {

    namespace Internal {
        class SynthService;
    }

    class SynthesisTask;
    class SynthesisTaskManagerPrivate;

    class SYNTH_EXPORT SynthesisTaskManager : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SynthesisTaskManager)
        Q_PROPERTY(int runningTaskCount READ runningTaskCount NOTIFY taskCountsChanged)
        Q_PROPERTY(int queuedTaskCount READ queuedTaskCount NOTIFY taskCountsChanged)

    public:
        ~SynthesisTaskManager() override;

        SynthesisTask *enqueue(const SynthesisTaskRequest &request, const SynthesisTaskOptions &options = {});
        QList<SynthesisTask *> tasks() const;
        QList<SynthesisTask *> tasksForService(const QUuid &serviceInstanceId) const;
        int runningTaskCount() const;
        int queuedTaskCount() const;
        qint64 cacheSize() const;
        qint64 cacheSize(SynthesisTaskType type) const;
        QString diagnosticsDirectory() const;

        bool setPriority(SynthesisTask *task, int priority);
        bool cancel(SynthesisTask *task);
        bool removeFinishedTask(SynthesisTask *task);
        void clearFinishedTasks();
        void clearFailedTasks();
        void clearCache();
        void clearCache(const QList<SynthesisTaskType> &types);
        void clearDiagnostics();
        void reloadSettings();
        void shutdown();

    Q_SIGNALS:
        void taskAdded(Synth::SynthesisTask *task);
        void taskChanged(Synth::SynthesisTask *task);
        void taskRemoved(Synth::SynthesisTask *task);
        void taskCountsChanged();
        void serviceTaskCountsChanged(const QUuid &serviceInstanceId);

    private:
        explicit SynthesisTaskManager(QObject *parent = nullptr);

        QScopedPointer<SynthesisTaskManagerPrivate> d_ptr;

        friend class Internal::SynthService;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISTASKMANAGER_H
