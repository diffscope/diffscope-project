#ifndef DIFFSCOPE_DSPX_MODEL_VIBRATO_H
#define DIFFSCOPE_DSPX_MODEL_VIBRATO_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxmodel/Handle.h>

namespace QDspx {
    struct Vibrato;
}

namespace dspx {

    class Model;
    class ModelPrivate;
    class VibratoPoints;

    class VibratoPrivate;

    class DSPX_MODEL_EXPORT Vibrato : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Vibrato)
        Q_PROPERTY(int amp READ amp WRITE setAmp NOTIFY ampChanged)
        Q_PRIVATE_PROPERTY(d_func(), double end MEMBER end WRITE setEnd NOTIFY endChanged)
        Q_PRIVATE_PROPERTY(d_func(), double freq MEMBER freq WRITE setFreq NOTIFY freqChanged)
        Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)
        Q_PRIVATE_PROPERTY(d_func(), double phase MEMBER phase WRITE setPhase NOTIFY phaseChanged)
        Q_PROPERTY(VibratoPoints *points READ points CONSTANT)
        Q_PRIVATE_PROPERTY(d_func(), double start MEMBER start WRITE setStart NOTIFY startChanged)

    public:
        ~Vibrato() override;

        int amp() const;
        void setAmp(int amp);

        double end() const;
        void setEnd(double end);

        double freq() const;
        void setFreq(double freq);

        int offset() const;
        void setOffset(int offset);

        double phase() const;
        void setPhase(double phase);

        VibratoPoints *points() const;

        double start() const;
        void setStart(double start);

        QDspx::Vibrato toQDspx() const;
        void fromQDspx(const QDspx::Vibrato &vibrato);

    Q_SIGNALS:
        void ampChanged(int amp);
        void endChanged(double end);
        void freqChanged(double freq);
        void offsetChanged(int offset);
        void phaseChanged(double phase);
        void startChanged(double start);

    private:
        friend class ModelPrivate;
        explicit Vibrato(Handle handle, Model *model);
        void handleProxySetEntityProperty(int property, const QVariant &value);
        QScopedPointer<VibratoPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_VIBRATO_H
