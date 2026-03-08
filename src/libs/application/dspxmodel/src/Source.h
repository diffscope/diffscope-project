#ifndef DIFFSCOPE_DSPX_MODEL_SOURCE_H
#define DIFFSCOPE_DSPX_MODEL_SOURCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

class QJsonObject;

namespace dspx {

    class SourcePrivate;

    class DSPX_MODEL_EXPORT Source : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Source);
        Q_PROPERTY(QJsonObject jsonObject READ jsonObject WRITE setJsonObject NOTIFY jsonObjectChanged)
    public:
        ~Source() override;

        QJsonObject jsonObject() const;
        void setJsonObject(const QJsonObject &jsonObject);

    Q_SIGNALS:
        void jsonObjectChanged(const QJsonObject &jsonObject);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit Source(Handle handle, Model *model);
        QScopedPointer<SourcePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_SOURCE_H
