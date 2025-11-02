#ifndef DIFFSCOPE_DSPX_MODEL_PRONUNCIATION_H
#define DIFFSCOPE_DSPX_MODEL_PRONUNCIATION_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxmodel/Handle.h>

namespace QDspx {
    struct Pronunciation;
}

namespace dspx {

    class Model;
    class ModelPrivate;

    class PronunciationPrivate;

    class DSPX_MODEL_EXPORT Pronunciation : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Pronunciation)
        Q_PROPERTY(QString original READ original WRITE setOriginal NOTIFY originalChanged)
        Q_PROPERTY(QString edited READ edited WRITE setEdited NOTIFY editedChanged)

    public:
        ~Pronunciation() override;

        QString original() const;
        void setOriginal(const QString &original);

        QString edited() const;
        void setEdited(const QString &edited);

        QDspx::Pronunciation toQDspx() const;
        void fromQDspx(const QDspx::Pronunciation &pronunciation);

    Q_SIGNALS:
        void originalChanged(const QString &original);
        void editedChanged(const QString &edited);

    private:
        friend class ModelPrivate;
        explicit Pronunciation(Handle handle, Model *model);
        void handleProxySetEntityProperty(int property, const QVariant &value);
        QScopedPointer<PronunciationPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PRONUNCIATION_H
