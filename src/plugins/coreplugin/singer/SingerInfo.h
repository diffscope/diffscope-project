// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_SINGERINFO_H
#define DIFFSCOPE_COREPLUGIN_SINGERINFO_H

#include <QJsonValue>
#include <QMap>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <coreplugin/coreglobal.h>

namespace Core {

    class SingerInfoData;

    struct CORE_EXPORT SingerLanguageInfo {
        Q_GADGET
        Q_PROPERTY(QString name MEMBER name)
        Q_PROPERTY(QString defaultLyric MEMBER defaultLyric)

    public:
        QString name;
        QString defaultLyric;

        bool operator==(const SingerLanguageInfo &) const = default;
        bool operator!=(const SingerLanguageInfo &) const = default;
    };

    class CORE_EXPORT SingerInfo {
        Q_GADGET
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(QUrl avatarUrl READ avatarUrl WRITE setAvatarUrl)
        Q_PROPERTY(QUrl backgroundUrl READ backgroundUrl WRITE setBackgroundUrl)
        Q_PROPERTY(QString defaultLanguage READ defaultLanguage WRITE setDefaultLanguage)
        Q_PROPERTY(LanguageMap languages READ languages WRITE setLanguages)
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

Q_DECLARE_METATYPE(Core::SingerLanguageInfo)
Q_DECLARE_METATYPE(Core::SingerInfo::LanguageMap)
Q_DECLARE_METATYPE(Core::SingerInfo)

#endif // DIFFSCOPE_COREPLUGIN_SINGERINFO_H
