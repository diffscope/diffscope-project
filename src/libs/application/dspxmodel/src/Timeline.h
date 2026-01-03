#ifndef DIFFSCOPE_DSPX_MODEL_TIMELINE_H
#define DIFFSCOPE_DSPX_MODEL_TIMELINE_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxmodel/DspxModelGlobal.h>

namespace QDspx {
    struct Timeline;
}

namespace dspx {

    class Model;

    class LabelSequence;
    class TempoSequence;
    class TimeSignatureSequence;

    class TimelinePrivate;

    class DSPX_MODEL_EXPORT Timeline : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Timeline)
        Q_PROPERTY(LabelSequence *labels READ labels CONSTANT)
        Q_PROPERTY(TempoSequence *tempos READ tempos CONSTANT)
        Q_PROPERTY(TimeSignatureSequence *timeSignatures READ timeSignatures CONSTANT)
        Q_PROPERTY(bool loopEnabled READ isLoopEnabled WRITE setLoopEnabled NOTIFY loopEnabledChanged)
        Q_PRIVATE_PROPERTY(d_func(), int loopStart MEMBER loopStart WRITE setLoopStart NOTIFY loopStartChanged)
        Q_PRIVATE_PROPERTY(d_func(), int loopLength MEMBER loopLength WRITE setLoopLength NOTIFY loopLengthChanged)
    public:
        ~Timeline() override;

        LabelSequence *labels() const;
        TempoSequence *tempos() const;
        TimeSignatureSequence *timeSignatures() const;

        bool isLoopEnabled() const;
        void setLoopEnabled(bool enabled);

        int loopStart() const;
        void setLoopStart(int loopStart);

        int loopLength() const;
        void setLoopLength(int loopLength);

        QDspx::Timeline toQDspx() const;
        void fromQDspx(const QDspx::Timeline &timeline);

    Q_SIGNALS:
        void loopEnabledChanged(bool enabled);
        void loopStartChanged(int loopStart);
        void loopLengthChanged(int loopLength);

    private:
        friend class ModelPrivate;
        explicit Timeline(Model *model);
        void handleProxySetEntityProperty(int property, const QVariant &value);

        QScopedPointer<TimelinePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TIMELINE_H
