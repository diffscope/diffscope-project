// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#include "LibreSVIPTypes.h"

#include <algorithm>

#include <QJsonArray>

namespace LibreSVIPFormatConverter::Internal {

    static QJsonArray pluginListToJson(const QList<LibreSVIPPluginInfo> &plugins) {
        QJsonArray array;
        for (const auto &plugin : plugins)
            array.append(plugin.toJson());
        return array;
    }

    static QList<LibreSVIPPluginInfo> pluginListFromJson(const QJsonValue &value) {
        QList<LibreSVIPPluginInfo> plugins;
        for (const auto &item : value.toArray()) {
            if (item.isObject())
                plugins.append(LibreSVIPPluginInfo::fromJson(item.toObject()));
        }
        return plugins;
    }

    QJsonObject LibreSVIPPluginInfo::toJson() const {
        QJsonArray suffixArray;
        for (const auto &suffix : suffixes)
            suffixArray.append(suffix);
        return {
            {QStringLiteral("identifier"), identifier},
            {QStringLiteral("name"), name},
            {QStringLiteral("version"), version},
            {QStringLiteral("description"), description},
            {QStringLiteral("author"), author},
            {QStringLiteral("website"), website},
            {QStringLiteral("jsonSchema"), jsonSchema},
            {QStringLiteral("fileFormat"), fileFormat},
            {QStringLiteral("suffixes"), suffixArray},
            {QStringLiteral("iconBase64"), iconBase64},
        };
    }

    LibreSVIPPluginInfo LibreSVIPPluginInfo::fromJson(const QJsonObject &object) {
        LibreSVIPPluginInfo plugin;
        plugin.identifier = object.value(QStringLiteral("identifier")).toString();
        plugin.name = object.value(QStringLiteral("name")).toString();
        plugin.version = object.value(QStringLiteral("version")).toString();
        plugin.description = object.value(QStringLiteral("description")).toString();
        plugin.author = object.value(QStringLiteral("author")).toString();
        plugin.website = object.value(QStringLiteral("website")).toString();
        plugin.jsonSchema = object.value(QStringLiteral("jsonSchema")).toObject();
        plugin.fileFormat = object.value(QStringLiteral("fileFormat")).toString();
        for (const auto &suffix : object.value(QStringLiteral("suffixes")).toArray())
            plugin.suffixes.append(suffix.toString());
        plugin.iconBase64 = object.value(QStringLiteral("iconBase64")).toString();
        return plugin;
    }

    bool LibreSVIPPluginCatalog::isComplete() const {
        return !inputs.isEmpty() && !outputs.isEmpty() &&
               find(LibreSVIPPluginCategory::Input, QStringLiteral("dspx")) &&
               find(LibreSVIPPluginCategory::Output, QStringLiteral("dspx"));
    }

    const LibreSVIPPluginInfo *LibreSVIPPluginCatalog::find(LibreSVIPPluginCategory category, const QString &identifier) const {
        const auto &items = plugins(category);
        const auto it = std::find_if(items.cbegin(), items.cend(), [&identifier](const LibreSVIPPluginInfo &plugin) {
            return plugin.identifier == identifier;
        });
        return it == items.cend() ? nullptr : &*it;
    }

    const QList<LibreSVIPPluginInfo> &LibreSVIPPluginCatalog::plugins(LibreSVIPPluginCategory category) const {
        switch (category) {
            case LibreSVIPPluginCategory::Input:
                return inputs;
            case LibreSVIPPluginCategory::Output:
                return outputs;
            case LibreSVIPPluginCategory::Middleware:
                return middlewares;
        }
        Q_UNREACHABLE_RETURN(inputs);
    }

    QJsonObject LibreSVIPPluginCatalog::toJson() const {
        return {
            {QStringLiteral("inputs"), pluginListToJson(inputs)},
            {QStringLiteral("outputs"), pluginListToJson(outputs)},
            {QStringLiteral("middlewares"), pluginListToJson(middlewares)},
        };
    }

    LibreSVIPPluginCatalog LibreSVIPPluginCatalog::fromJson(const QJsonObject &object) {
        LibreSVIPPluginCatalog catalog;
        catalog.inputs = pluginListFromJson(object.value(QStringLiteral("inputs")));
        catalog.outputs = pluginListFromJson(object.value(QStringLiteral("outputs")));
        catalog.middlewares = pluginListFromJson(object.value(QStringLiteral("middlewares")));
        return catalog;
    }

}
