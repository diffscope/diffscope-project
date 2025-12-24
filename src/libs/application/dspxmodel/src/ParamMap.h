#ifndef DIFFSCOPE_DSPX_MODEL_PARAMMAP_H
#define DIFFSCOPE_DSPX_MODEL_PARAMMAP_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Param;
    using Params = QMap<QString, Param>;
}

namespace dspx {

    class SingingClip;
    class Param;
    class ParamMapPrivate;

    class DSPX_MODEL_EXPORT ParamMap : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ParamMap)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QStringList keys READ keys NOTIFY keysChanged)
        Q_PROPERTY(QList<Param *> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(SingingClip *singingClip READ singingClip CONSTANT)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)

    public:
        ~ParamMap() override;

        int size() const;
        QStringList keys() const;
        QList<Param *> items() const;
        Q_INVOKABLE bool insertItem(const QString &key, Param *item);
        Q_INVOKABLE Param *removeItem(const QString &key);
        Q_INVOKABLE Param *item(const QString &key) const;
        Q_INVOKABLE bool contains(const QString &key) const;

        QDspx::Params toQDspx() const;
        void fromQDspx(const QDspx::Params &paramMap);

        SingingClip *singingClip() const;

    Q_SIGNALS:
        void itemAboutToInsert(const QString &key, Param *item);
        void itemInserted(const QString &key, Param *item);
        void itemAboutToRemove(const QString &key, Param *item);
        void itemRemoved(const QString &key, Param *item);
        void sizeChanged(int size);
        void keysChanged();
        void itemsChanged();

    protected:
        void handleInsertIntoMapContainer(Handle entity, const QString &key) override;
        void handleTakeFromMapContainer(Handle takenEntity, const QString &key) override;

    private:
        friend class ModelPrivate;
        explicit ParamMap(SingingClip *singingClip, Handle handle, Model *model);
        QScopedPointer<ParamMapPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAMMAP_H
