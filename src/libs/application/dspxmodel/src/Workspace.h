#ifndef DIFFSCOPE_DSPX_MODEL_WORKSPACE_H
#define DIFFSCOPE_DSPX_MODEL_WORKSPACE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace dspx {

    class WorkspaceInfo;

    class WorkspacePrivate;

    class Workspace : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Workspace)

    public:
        ~Workspace() override;

    protected:
        void handleInsertIntoMapContainer(Handle entity, const QString &key) override;
        void handleTakeFromMapContainer(Handle takenEntity, const QString &key) override;

    private:
        friend class ModelPrivate;
        explicit Workspace(Handle handle, Model *model);
        QScopedPointer<WorkspacePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_WORKSPACE_H
