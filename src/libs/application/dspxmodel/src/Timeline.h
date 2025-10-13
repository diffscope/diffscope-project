#ifndef DIFFSCOPE_DSPX_MODEL_TIMELINE_H
#define DIFFSCOPE_DSPX_MODEL_TIMELINE_H

#include <QObject>
#include <qqmlintegration.h>

#include <dspxmodel/DspxModelGlobal.h>

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
        Q_PROPERTY(LabelSequence *labelSequence READ labelSequence CONSTANT)
    public:
        ~Timeline() override;

        LabelSequence *labelSequence() const;

    private:
        friend class ModelPrivate;
        explicit Timeline(Model *model);

        QScopedPointer<TimelinePrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TIMELINE_H
