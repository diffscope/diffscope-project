#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_PREBUILTINFORMATS_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_PREBUILTINFORMATS_H

#include <QString>
#include <QPair>
#include <QList>

namespace LibreSVIPFormatConverter::Internal {

    // These are used when libresvip-cli is not loaded, and do not need translations

    inline QList<QPair<QString, QString>> preBuiltInFormats() {
        return {
            {QStringLiteral("ACE Studio project File"), QStringLiteral("*.acep")},
            // TODO ...
        };
    }

    inline QStringList preBuiltInFormatHeuristicFilters() {
        QStringList a;
        for (const auto &[_, filter] : preBuiltInFormats()) {
            a.append(filter);
        }
        return a;
    }

    inline QStringList preBuiltInFormatFileDialogFilters() {
        QStringList a;
        for (const auto &[name, filter] : preBuiltInFormats()) {
            a.append(QStringLiteral("%1 (%2)").arg(name, filter));
        }
        a.append(QStringLiteral("All Supported formats (%1)").arg(preBuiltInFormatHeuristicFilters().join(" ")));
        return a;
    }

}

#endif //DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_PREBUILTINFORMATS_H
