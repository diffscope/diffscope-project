#ifndef DIFFSCOPE_DSPX_MODEL_PARAM_H
#define DIFFSCOPE_DSPX_MODEL_PARAM_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Param;
}

namespace dspx {

    class ParamCurveSequence;
    class ParamMap;
    class ParamPrivate;

    class DSPX_MODEL_EXPORT Param : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Param)
        Q_PROPERTY(ParamCurveSequence *original READ original CONSTANT)
        Q_PROPERTY(ParamCurveSequence *transform READ transform CONSTANT)
        Q_PROPERTY(ParamCurveSequence *edited READ edited CONSTANT)
        Q_PROPERTY(ParamMap *paramMap READ paramMap CONSTANT)

    public:
        ~Param() override;

        ParamCurveSequence *original() const;
        ParamCurveSequence *transform() const;
        ParamCurveSequence *edited() const;

        ParamMap *paramMap() const;

        QDspx::Param toQDspx() const;
        void fromQDspx(const QDspx::Param &param);

    Q_SIGNALS:
        void paramMapChanged();

    private:
        friend class ModelPrivate;
        explicit Param(Handle handle, Model *model);
        QScopedPointer<ParamPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAM_H
