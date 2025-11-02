#ifndef DIFFSCOPE_DSPX_MODEL_VIBRATOPOINTS_H
#define DIFFSCOPE_DSPX_MODEL_VIBRATOPOINTS_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxmodel/Handle.h>

namespace QDspx {
    struct VibratoPoints;
}

namespace dspx {

    class Model;
    class ModelPrivate;
    class VibratoPointDataArray;

    class VibratoPointsPrivate;

    class DSPX_MODEL_EXPORT VibratoPoints : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(VibratoPoints)
        Q_PROPERTY(VibratoPointDataArray *amp READ amp CONSTANT)
        Q_PROPERTY(VibratoPointDataArray *freq READ freq CONSTANT)

    public:
        ~VibratoPoints() override;

        VibratoPointDataArray *amp() const;
        VibratoPointDataArray *freq() const;

        QDspx::VibratoPoints toQDspx() const;
        void fromQDspx(const QDspx::VibratoPoints &vibratoPoints);

    private:
        friend class ModelPrivate;
        explicit VibratoPoints(Handle handle, Model *model);
        QScopedPointer<VibratoPointsPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_VIBRATOPOINTS_H
