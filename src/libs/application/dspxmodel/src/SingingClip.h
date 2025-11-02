#ifndef DIFFSCOPE_DSPX_MODEL_SINGINGCLIP_H
#define DIFFSCOPE_DSPX_MODEL_SINGINGCLIP_H

#include <qqmlintegration.h>

#include <dspxmodel/Clip.h>

namespace QDspx {
    struct SingingClip;
}

namespace dspx {

    class NoteSequence;
    class ParamMap;
    class SourceMap;
    class SingingClipPrivate;

    class DSPX_MODEL_EXPORT SingingClip : public Clip {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(SingingClip)
        Q_PROPERTY(NoteSequence *notes READ notes CONSTANT)
        Q_PROPERTY(ParamMap *params READ params CONSTANT)
        Q_PROPERTY(SourceMap *sources READ sources CONSTANT)

    public:
        ~SingingClip() override;

        NoteSequence *notes() const;
        ParamMap *params() const;
        SourceMap *sources() const;

        QDspx::SingingClip toQDspx() const;
        void fromQDspx(const QDspx::SingingClip &clip);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit SingingClip(Handle handle, Model *model);
        QScopedPointer<SingingClipPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_SINGINGCLIP_H
