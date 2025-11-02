#ifndef DIFFSCOPE_DSPX_MODEL_ENTITYOBJECT_H
#define DIFFSCOPE_DSPX_MODEL_ENTITYOBJECT_H

#include <QObject>

#include <dspxmodel/DspxModelGlobal.h>
#include <dspxmodel/Handle.h>

namespace dspx {

    class Model;
    class ModelPrivate;

    class EntityObjectPrivate;

    class DSPX_MODEL_EXPORT EntityObject : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(EntityObject);
        Q_PROPERTY(Model *model READ model CONSTANT)
    public:
        ~EntityObject() override;

        Handle handle() const;

        Model *model() const;

    protected:
        virtual void handleInsertIntoSequenceContainer(Handle entity);
        virtual void handleInsertIntoListContainer(Handle entities, int index);
        virtual void handleInsertIntoMapContainer(Handle entity, const QString &key);

        virtual void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity);
        virtual void handleTakeFromListContainer(Handle takenEntities, int index);
        virtual void handleTakeFromMapContainer(Handle takenEntity, const QString &key);
        virtual void handleRotateListContainer(int leftIndex, int middleIndex, int rightIndex);

        virtual void handleSetEntityProperty(int property, const QVariant &value);

        virtual void handleSpliceDataArray(int index, int length, const QVariantList &values);
        virtual void handleRotateDataArray(int leftIndex, int middleIndex, int rightIndex);

        explicit EntityObject(Handle handle, Model *model);

    private:
        friend class Model;
        friend class ModelPrivate;
        explicit EntityObject(QObject *parent = nullptr);
        QScopedPointer<EntityObjectPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ENTITYOBJECT_H
