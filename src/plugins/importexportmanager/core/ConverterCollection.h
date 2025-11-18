#ifndef CONVERTERCOLLECTION_H
#define CONVERTERCOLLECTION_H

#include <qqmlintegration.h>

#include <CoreApi/objectpool.h>

class QQmlEngine;
class QJSEngine;

namespace ImportExportManager {

    class FileConverter;
    class ClipboardConverter;

    class ConverterCollection : public Core::ObjectPool {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(QList<FileConverter *> fileConverters READ fileConverters CONSTANT)
        Q_PROPERTY(QList<ClipboardConverter *> clipboardConverters READ clipboardConverters CONSTANT)
    public:
        ~ConverterCollection() override;

        static ConverterCollection *instance();

        static inline ConverterCollection *create(QQmlEngine *, QJSEngine *engine) {
            return instance();
        }

        QList<FileConverter *> fileConverters() const;
        QList<ClipboardConverter *> clipboardConverters() const;

    private:
        explicit ConverterCollection(QObject *parent);

    };

}

#endif //CONVERTERCOLLECTION_H
