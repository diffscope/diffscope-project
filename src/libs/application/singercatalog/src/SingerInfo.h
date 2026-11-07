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

    /** @brief Stores a singer language's display name and default lyric. */
    struct SINGERCATALOG_EXPORT SingerLanguageInfo {
        Q_GADGET
        Q_PROPERTY(QString name MEMBER name)
        Q_PROPERTY(QString defaultLyric MEMBER defaultLyric)

    public:
        QString name; ///< Contains the user-visible language name. An empty name permits display of the language ID.
        QString defaultLyric; ///< Contains the default lyric. An empty lyric permits the manager's fallback.

        /** @brief Returns whether the name and default lyric are equal. */
        bool operator==(const SingerLanguageInfo &) const = default;
        /** @brief Returns whether the name or default lyric differs. */
        bool operator!=(const SingerLanguageInfo &) const = default;
    };

    /** @brief Stores implicitly shared singer metadata independently of the document model. */
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
        /** @brief Maps language identifiers to their metadata. */
        using LanguageMap = QMap<QString, SingerLanguageInfo>;

        /** @brief Constructs empty singer metadata. */
        SingerInfo();
        /** @brief Constructs a shared copy of @p other. */
        SingerInfo(const SingerInfo &other);
        /** @brief Moves metadata from @p other. */
        SingerInfo(SingerInfo &&other) noexcept;
        /** @brief Assigns a shared copy of @p other and returns this object. */
        SingerInfo &operator=(const SingerInfo &other);
        /** @brief Moves metadata from @p other and returns this object. */
        SingerInfo &operator=(SingerInfo &&other) noexcept;
        /** @brief Releases the shared metadata. */
        ~SingerInfo();

        /** @brief Returns the user-visible singer name. */
        QString name() const;
        /** @brief Sets the user-visible singer name. */
        void setName(const QString &name);

        /** @brief Returns the singer avatar URL. */
        QUrl avatarUrl() const;
        /** @brief Sets the singer avatar URL. */
        void setAvatarUrl(const QUrl &avatarUrl);

        /** @brief Returns the singer background image URL. */
        QUrl backgroundUrl() const;
        /** @brief Sets the singer background image URL. */
        void setBackgroundUrl(const QUrl &backgroundUrl);

        /** @brief Returns the default language ID, or an empty ID to select the first language in key order. */
        QString defaultLanguage() const;
        /** @brief Sets the default language ID, or selects the first language in key order if empty. */
        void setDefaultLanguage(const QString &defaultLanguage);

        /** @brief Returns the supported language metadata indexed by language identifier. */
        LanguageMap languages() const;
        /** @brief Sets the supported language metadata indexed by language identifier. */
        void setLanguages(const LanguageMap &languages);

        /** @brief Returns the parameter identifiers supported by this singer. */
        QStringList supportedParameters() const;
        /** @brief Sets the parameter identifiers supported by this singer. */
        void setSupportedParameters(const QStringList &supportedParameters);

        /** @brief Returns the mixing compatibility group. An empty group disables mixing. */
        QString mixGroup() const;
        /** @brief Sets the mixing compatibility group shared by compatible singers. An empty group disables mixing. */
        void setMixGroup(const QString &mixGroup);

        /** @brief Returns application-specific singer metadata. */
        QVariant userData() const;
        /** @brief Sets application-specific singer metadata. */
        void setUserData(const QVariant &userData);

        /** @brief Returns the default extra data for new document singer entries. */
        QJsonValue defaultExtra() const;
        /** @brief Sets the default extra data for new document singer entries. */
        void setDefaultExtra(const QJsonValue &defaultExtra);

        /** @brief Returns whether all metadata fields are equal. */
        bool operator==(const SingerInfo &other) const;
        /** @brief Returns whether any metadata field differs. */
        bool operator!=(const SingerInfo &other) const;

    private:
        QSharedDataPointer<SingerInfoData> d;
    };

}

Q_DECLARE_METATYPE(SingerCatalog::SingerLanguageInfo)
Q_DECLARE_METATYPE(SingerCatalog::SingerInfo::LanguageMap)
Q_DECLARE_METATYPE(SingerCatalog::SingerInfo)

#endif // DIFFSCOPE_SINGERCATALOG_SINGERINFO_H
