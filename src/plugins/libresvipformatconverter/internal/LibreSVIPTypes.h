#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPTYPES_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPTYPES_H

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace LibreSVIPFormatConverter::Internal {

    enum class LibreSVIPPluginCategory {
        Input,
        Output,
        Middleware,
    };

    struct LibreSVIPPluginInfo {
        QString identifier;
        QString name;
        QString version;
        QString description;
        QString author;
        QString website;
        QJsonObject jsonSchema;
        QString fileFormat;
        QStringList suffixes;
        QString iconBase64;

        QJsonObject toJson() const;
        static LibreSVIPPluginInfo fromJson(const QJsonObject &object);
    };

    struct LibreSVIPPluginCatalog {
        QList<LibreSVIPPluginInfo> inputs;
        QList<LibreSVIPPluginInfo> outputs;
        QList<LibreSVIPPluginInfo> middlewares;

        bool isComplete() const;
        const LibreSVIPPluginInfo *find(LibreSVIPPluginCategory category, const QString &identifier) const;
        const QList<LibreSVIPPluginInfo> &plugins(LibreSVIPPluginCategory category) const;

        QJsonObject toJson() const;
        static LibreSVIPPluginCatalog fromJson(const QJsonObject &object);
    };

    struct LibreSVIPValidationResult {
        bool success{};
        bool cancelled{};
        QByteArray sha512;
        LibreSVIPPluginCatalog catalog;
        QString errorMessage;
    };

    struct LibreSVIPConversionRequestData {
        QString inputIdentifier;
        QString outputIdentifier;
        QByteArray inputData;
        QJsonObject inputOptions;
        QJsonObject outputOptions;
        QHash<QString, QJsonObject> middlewareOptions;
    };

    struct LibreSVIPConversionResult {
        bool success{};
        bool cancelled{};
        QList<QByteArray> outputData;
        QStringList warningMessages;
        QString errorMessage;
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPTYPES_H
