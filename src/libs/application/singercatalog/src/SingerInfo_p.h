// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_SINGERINFO_P_H
#define DIFFSCOPE_SINGERCATALOG_SINGERINFO_P_H

#include <singercatalog/SingerInfo.h>

#include <QSharedData>

namespace SingerCatalog {

    class SingerInfoData : public QSharedData {
    public:
        QString name;
        QUrl avatarUrl;
        QUrl backgroundUrl;
        QString defaultLanguage;
        SingerInfo::LanguageMap languages;
        QStringList supportedParameters;
        QString mixGroup;
        QVariant userData;
        QJsonValue defaultExtra;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_SINGERINFO_P_H
