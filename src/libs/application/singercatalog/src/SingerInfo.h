// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_SINGERINFO_H
#define DIFFSCOPE_SINGERCATALOG_SINGERINFO_H

#include <QJsonValue>
#include <QMap>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>

#include <singercatalog/singercatalogglobal.h>

namespace SingerCatalog {

    class SingerInfoData;

    struct SINGERCATALOG_EXPORT SingerLanguageInfo {
        Q_GADGET
        Q_PROPERTY(QString name MEMBER name)
        Q_PROPERTY(QString defaultLyric MEMBER defaultLyric)

    public:
        QString name;
        QString defaultLyric;

        bool operator==(const SingerLanguageInfo &) const = default;
        bool operator!=(const SingerLanguageInfo &) const = default;
    };

    class SINGERCATALOG_EXPORT SingerInfo {
        Q_GADGET
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(QUrl avatarUrl READ avatarUrl WRITE setAvatarUrl)
        Q_PROPERTY(QUrl backgroundUrl READ backgroundUrl WRITE setBackgroundUrl)
        Q_PROPERTY(QString defaultLanguage READ defaultLanguage WRITE setDefaultLanguage)
        Q_PROPERTY(LanguageMap languages READ languages WRITE setLanguages)
        Q_PROPERTY(QStringList supportedParameters READ supportedParameters WRITE setSupportedParameters)
        Q_PROPERTY(QString mixGroup READ mixGroup WRITE setMixGroup)
        Q_PROPERTY(QVariant userData READ userData WRITE setUserData)
        Q_PROPERTY(QJsonValue defaultExtra READ defaultExtra WRITE setDefaultExtra)

    public:
        using LanguageMap = QMap<QString, SingerLanguageInfo>;

        SingerInfo();
        SingerInfo(const SingerInfo &other);
        SingerInfo(SingerInfo &&other) noexcept;
        SingerInfo &operator=(const SingerInfo &other);
        SingerInfo &operator=(SingerInfo &&other) noexcept;
        ~SingerInfo();

        QString name() const;
        void setName(const QString &name);

        QUrl avatarUrl() const;
        void setAvatarUrl(const QUrl &avatarUrl);

        QUrl backgroundUrl() const;
        void setBackgroundUrl(const QUrl &backgroundUrl);

        QString defaultLanguage() const;
        void setDefaultLanguage(const QString &defaultLanguage);

        LanguageMap languages() const;
        void setLanguages(const LanguageMap &languages);

        QStringList supportedParameters() const;
        void setSupportedParameters(const QStringList &supportedParameters);

        QString mixGroup() const;
        void setMixGroup(const QString &mixGroup);

        QVariant userData() const;
        void setUserData(const QVariant &userData);

        QJsonValue defaultExtra() const;
        void setDefaultExtra(const QJsonValue &defaultExtra);

        bool operator==(const SingerInfo &other) const;
        bool operator!=(const SingerInfo &other) const;

    private:
        QSharedDataPointer<SingerInfoData> d;
    };

}

Q_DECLARE_METATYPE(SingerCatalog::SingerLanguageInfo)
Q_DECLARE_METATYPE(SingerCatalog::SingerInfo::LanguageMap)
Q_DECLARE_METATYPE(SingerCatalog::SingerInfo)

#endif // DIFFSCOPE_SINGERCATALOG_SINGERINFO_H
