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

    class ClipTime : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ClipTime)
        Q_PROPERTY(int start READ start WRITE setStart NOTIFY startChanged)
        Q_PRIVATE_PROPERTY(d_func(), int length MEMBER length WRITE setLength NOTIFY lengthChanged)
        Q_PRIVATE_PROPERTY(d_func(), int clipStart MEMBER clipStart WRITE setClipStart NOTIFY clipStartChanged)
        Q_PRIVATE_PROPERTY(d_func(), int clipLen MEMBER clipLen WRITE setClipLen NOTIFY clipLenChanged)

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
