#ifndef DIFFSCOPE_DSPX_MODEL_SOURCEMAP_H
#define DIFFSCOPE_DSPX_MODEL_SOURCEMAP_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    using Sources = QMap<QString, QJsonObject>;
}

namespace dspx {

    class Source;
    class SourceMapPrivate;

    class DSPX_MODEL_EXPORT SourceMap : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(SourceMap)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QStringList keys READ keys NOTIFY keysChanged)
        Q_PROPERTY(QList<Source *> items READ items NOTIFY itemsChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)

    public:
        ~SourceMap() override;

        int size() const;
        QStringList keys() const;
        QList<Source *> items() const;
        Q_INVOKABLE bool insertItem(const QString &key, Source *item);
        Q_INVOKABLE Source *removeItem(const QString &key);
        Q_INVOKABLE Source *item(const QString &key) const;
        Q_INVOKABLE bool contains(const QString &key) const;

        QDspx::Sources toQDspx() const;
        void fromQDspx(const QDspx::Sources &sourceMap);

    Q_SIGNALS:
        void itemAboutToInsert(const QString &key, Source *item);
        void itemInserted(const QString &key, Source *item);
        void itemAboutToRemove(const QString &key, Source *item);
        void itemRemoved(const QString &key, Source *item);
        void sizeChanged(int size);
        void keysChanged();
        void itemsChanged();

    protected:
        void handleInsertIntoMapContainer(Handle entity, const QString &key) override;
        void handleTakeFromMapContainer(Handle takenEntity, const QString &key) override;

    private:
        friend class ModelPrivate;
        explicit SourceMap(Handle handle, Model *model);
        QScopedPointer<SourceMapPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_SOURCEMAP_H