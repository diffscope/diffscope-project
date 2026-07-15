#ifndef DIFFSCOPE_COREPLUGIN_SINGERINFO_P_H
#define DIFFSCOPE_COREPLUGIN_SINGERINFO_P_H

#include <coreplugin/SingerInfo.h>

#include <QSharedData>

namespace Core {

    class SingerInfoData : public QSharedData {
    public:
        QString name;
        QUrl avatarUrl;
        QUrl backgroundUrl;
        QString defaultLanguage;
        QStringList languages;
        QString mixGroup;
        QVariant userData;
        QJsonValue defaultExtra;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_SINGERINFO_P_H
