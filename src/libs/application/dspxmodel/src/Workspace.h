#ifndef DIFFSCOPE_DSPX_MODEL_WORKSPACE_H
#define DIFFSCOPE_DSPX_MODEL_WORKSPACE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    using Workspace = QMap<QString, QJsonObject>;
}

namespace dspx {

    class WorkspaceInfo;

    class WorkspacePrivate;

    class DSPX_MODEL_EXPORT Workspace : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Workspace)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QStringList keys READ keys NOTIFY keysChanged)
        Q_PROPERTY(QList<WorkspaceInfo *> items READ items NOTIFY itemsChanged)

    public:
        ~Workspace() override;

        int size() const;
        QStringList keys() const;
        QList<WorkspaceInfo *> items() const;
        Q_INVOKABLE void insertItem(const QString &key, WorkspaceInfo *item);
        Q_INVOKABLE WorkspaceInfo *removeItem(const QString &key);
        Q_INVOKABLE WorkspaceInfo *item(const QString &key) const;
        Q_INVOKABLE bool contains(const QString &key) const;

        QDspx::Workspace toQDspx() const;
        void fromQDspx(const QDspx::Workspace &workspace);

    Q_SIGNALS:
        void itemAboutToInsert(const QString &key, WorkspaceInfo *item);
        void itemInserted(const QString &key, WorkspaceInfo *item);
        void itemAboutToRemove(const QString &key, WorkspaceInfo *item);
        void itemRemoved(const QString &key, WorkspaceInfo *item);
        void sizeChanged(int size);
        void keysChanged();
        void itemsChanged();

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
