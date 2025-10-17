#ifndef DIFFSCOPE_DSPX_MODEL_AUDIOCLIP_H
#define DIFFSCOPE_DSPX_MODEL_AUDIOCLIP_H

#include <qqmlintegration.h>

#include <dspxmodel/Clip.h>

namespace QDspx {
    struct AudioClip;
}

namespace dspx {

    class AudioClipPrivate;

    class DSPX_MODEL_EXPORT AudioClip : public Clip {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AudioClip)
        Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)

    public:
        ~AudioClip() override;

        QString path() const;
        void setPath(const QString &path);

        QDspx::AudioClip toQDspx() const;
        void fromQDspx(const QDspx::AudioClip &clip);

    Q_SIGNALS:
        void pathChanged(const QString &path);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit AudioClip(Handle handle, Model *model);
        QScopedPointer<AudioClipPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_AUDIOCLIP_H
