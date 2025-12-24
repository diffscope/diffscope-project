#ifndef DIFFSCOPE_DSPX_MODEL_PARAMCURVE_H
#define DIFFSCOPE_DSPX_MODEL_PARAMCURVE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct ParamCurve;
    using ParamCurveRef = QSharedPointer<ParamCurve>;
}

namespace dspx {

    class ParamCurveSequence;
    class ParamCurvePrivate;

    class DSPX_MODEL_EXPORT ParamCurve : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ParamCurve)
        Q_PROPERTY(int start READ start WRITE setStart NOTIFY startChanged)
        Q_PROPERTY(CurveType type READ type CONSTANT)
        Q_PROPERTY(ParamCurveSequence *paramCurveSequence READ paramCurveSequence NOTIFY paramCurveSequenceChanged)

    public:
        enum CurveType {
            Anchor,
            Free
        };
        Q_ENUM(CurveType)

        ~ParamCurve() override;

        int start() const;
        void setStart(int start);

        CurveType type() const;

        ParamCurveSequence *paramCurveSequence() const;

        QDspx::ParamCurveRef toQDspx() const;
        void fromQDspx(const QDspx::ParamCurveRef &curve);

    Q_SIGNALS:
        void startChanged(int start);
        void paramCurveSequenceChanged();

    protected:
        explicit ParamCurve(CurveType type, Handle handle, Model *model);
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        QScopedPointer<ParamCurvePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAMCURVE_H
