#ifndef DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_H
#define DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_H

#include <QObject>

#include <coreplugin/coreglobal.h>

namespace Core {

    class TrackColorSchemaPrivate;

    class CORE_EXPORT TrackColorSchema : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(TrackColorSchema)
        Q_PROPERTY(QList<QColor> colors READ colors WRITE setColors NOTIFY colorsChanged)

    public:
        explicit TrackColorSchema(QObject *parent = nullptr);
        ~TrackColorSchema() override;

        QList<QColor> colors() const;
        void setColors(const QList<QColor> &colors);

    Q_SIGNALS:
        void colorsChanged(const QList<QColor> &colors);

    private:
        QScopedPointer<TrackColorSchemaPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_H
