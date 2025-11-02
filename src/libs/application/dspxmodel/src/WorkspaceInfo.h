#ifndef DIFFSCOPE_DSPX_MODEL_WORKSPACEINFO_H
#define DIFFSCOPE_DSPX_MODEL_WORKSPACEINFO_H

#include <dspxmodel/EntityObject.h>
#include <qqmlintegration.h>

class QJsonObject;

namespace dspx {

    class WorkspaceInfoPrivate;

    class DSPX_MODEL_EXPORT WorkspaceInfo : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(WorkspaceInfo);
        Q_PROPERTY(QJsonObject jsonObject READ jsonObject WRITE setJsonObject NOTIFY jsonObjectChanged)
    public:
        ~WorkspaceInfo() override;

        QJsonObject jsonObject() const;
        void setJsonObject(const QJsonObject &jsonObject);

    Q_SIGNALS:
        void jsonObjectChanged(const QJsonObject &jsonObject);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit WorkspaceInfo(Handle handle, Model *model);
        QScopedPointer<WorkspaceInfoPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_WORKSPACEINFO_H