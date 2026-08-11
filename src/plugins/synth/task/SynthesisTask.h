#ifndef DIFFSCOPE_SYNTH_SYNTHESISTASK_H
#define DIFFSCOPE_SYNTH_SYNTHESISTASK_H

#include <QDateTime>
#include <QObject>
#include <QScopedPointer>
#include <QUuid>

#include <synth/SynthesisModel.h>
#include <synth/synthglobal.h>

namespace Synth {

    class SynthesisTaskManager;
    class SynthesisTaskPrivate;

    class SYNTH_EXPORT SynthesisTask : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SynthesisTask)
        Q_PROPERTY(QUuid id READ id CONSTANT)
        Q_PROPERTY(Synth::SynthesisTaskType type READ type CONSTANT)
        Q_PROPERTY(State state READ state NOTIFY stateChanged)
        Q_PROPERTY(QString displayName READ displayName CONSTANT)
        Q_PROPERTY(QUuid serviceInstanceId READ serviceInstanceId NOTIFY serviceChanged)
        Q_PROPERTY(QString serviceInstanceName READ serviceInstanceName NOTIFY serviceChanged)
        Q_PROPERTY(int priority READ priority NOTIFY priorityChanged)
        Q_PROPERTY(QDateTime createdAt READ createdAt CONSTANT)
        Q_PROPERTY(QDateTime startedAt READ startedAt NOTIFY stateChanged)
        Q_PROPERTY(QDateTime finishedAt READ finishedAt NOTIFY stateChanged)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)

    public:
        enum State {
            Queued,
            Running,
            Succeeded,
            Failed,
            Canceled,
        };
        Q_ENUM(State)

        ~SynthesisTask() override;

        QUuid id() const;
        SynthesisTaskType type() const;
        State state() const;
        QString displayName() const;
        QUuid serviceInstanceId() const;
        QString serviceInstanceName() const;
        int priority() const;
        QDateTime createdAt() const;
        QDateTime startedAt() const;
        QDateTime finishedAt() const;
        QString errorMessage() const;
        SynthesisTaskRequest request() const;
        SynthesisTaskOptions options() const;
        SynthesisTaskResult result() const;
        bool isFinished() const;

    Q_SIGNALS:
        void stateChanged();
        void serviceChanged();
        void priorityChanged();
        void finished();

    private:
        explicit SynthesisTask(const SynthesisTaskRequest &request, const SynthesisTaskOptions &options, QObject *parent = nullptr);

        QScopedPointer<SynthesisTaskPrivate> d_ptr;

        friend class SynthesisTaskManager;
        friend class SynthesisTaskManagerPrivate;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISTASK_H
