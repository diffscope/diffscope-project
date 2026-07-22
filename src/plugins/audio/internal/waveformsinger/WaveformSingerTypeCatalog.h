#ifndef DIFFSCOPE_AUDIO_WAVEFORMSINGERTYPECATALOG_H
#define DIFFSCOPE_AUDIO_WAVEFORMSINGERTYPECATALOG_H

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <qqmlintegration.h>

namespace Audio::Internal {

    class WaveformSingerTypeCatalog : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(QVariantList entries READ entries CONSTANT)

    public:
        struct Entry {
            QString type;
            QString name;
        };

        explicit WaveformSingerTypeCatalog(QObject *parent = nullptr);
        ~WaveformSingerTypeCatalog() override;

        QVariantList entries() const;

        static const QList<Entry> &availableTypes();
        static QString fallbackType();
        static QString normalizedType(const QString &type);
        static int indexOf(const QString &type);
    };

}

#endif // DIFFSCOPE_AUDIO_WAVEFORMSINGERTYPECATALOG_H
