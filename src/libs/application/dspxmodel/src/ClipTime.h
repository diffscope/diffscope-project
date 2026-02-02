#ifndef DIFFSCOPE_DSPX_MODEL_CLIPTIME_H
#define DIFFSCOPE_DSPX_MODEL_CLIPTIME_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxmodel/Handle.h>

namespace QDspx {
    struct ClipTime;
}

namespace dspx {

    class Model;
    class ModelPrivate;

    class ClipTimePrivate;

    class DSPX_MODEL_EXPORT ClipTime : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ClipTime)
        Q_PROPERTY(int start READ start WRITE setStart NOTIFY startChanged)
        Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
        Q_PROPERTY(int clipStart READ clipStart WRITE setClipStart NOTIFY clipStartChanged)
        Q_PROPERTY(int clipLen READ clipLen WRITE setClipLen NOTIFY clipLenChanged)

    public:
        ~ClipTime() override;

        int start() const;
        void setStart(int start);

        int length() const;
        void setLength(int length);

        int clipStart() const;
        void setClipStart(int clipStart);

        int clipLen() const;
        void setClipLen(int clipLen);

        QDspx::ClipTime toQDspx() const;
        void fromQDspx(const QDspx::ClipTime &clipTime);

    Q_SIGNALS:
        void startChanged(int start);
        void lengthChanged(int length);
        void clipStartChanged(int clipStart);
        void clipLenChanged(int clipLen);

    private:
        friend class ModelPrivate;
        explicit ClipTime(Handle handle, Model *model);
        void handleProxySetEntityProperty(int property, const QVariant &value);
        QScopedPointer<ClipTimePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIPTIME_H
