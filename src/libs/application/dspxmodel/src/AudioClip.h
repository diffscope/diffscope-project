#ifndef DIFFSCOPE_DSPX_MODEL_AUDIOCLIP_H
#define DIFFSCOPE_DSPX_MODEL_AUDIOCLIP_H

#include <QVariant>
#include <qqmlintegration.h>

#include <dspxmodel/Clip.h>

namespace opendspx {
    struct AudioClip;
}

namespace dspx {

    class AudioClipPrivate;

    struct DSPX_MODEL_EXPORT AudioPathInfo {
        Q_GADGET
        Q_PROPERTY(QString absoluteDir MEMBER absoluteDir)
        Q_PROPERTY(QString relativeDir MEMBER relativeDir)
        Q_PROPERTY(QString fileName MEMBER fileName)
        Q_PROPERTY(QString formatEntryClassName MEMBER formatEntryClassName)
        Q_PROPERTY(QVariant userData MEMBER userData)
    public:
        QString absoluteDir;
        QString relativeDir;
        QString fileName;
        QString formatEntryClassName;
        QVariant userData;
    };

    class DSPX_MODEL_EXPORT AudioClip : public Clip {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AudioClip)
        Q_PROPERTY(AudioPathInfo path READ path WRITE setPath NOTIFY pathChanged)

    public:
        ~AudioClip() override;

        AudioPathInfo path() const;
        void setPath(const AudioPathInfo &path);

        opendspx::AudioClip toOpenDspx() const;
        void fromOpenDspx(const opendspx::AudioClip &clip);

    Q_SIGNALS:
        void pathChanged(const AudioPathInfo &path);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit AudioClip(Handle handle, Model *model);
        QScopedPointer<AudioClipPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_AUDIOCLIP_H
