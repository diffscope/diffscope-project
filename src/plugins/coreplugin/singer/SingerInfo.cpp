#include "SingerInfo.h"
#include "SingerInfo_p.h"

namespace Core {

    SingerInfo::SingerInfo() : d(new SingerInfoData) {
    }

    SingerInfo::SingerInfo(const SingerInfo &other) = default;

    SingerInfo::SingerInfo(SingerInfo &&other) noexcept = default;

    SingerInfo &SingerInfo::operator=(const SingerInfo &other) = default;

    SingerInfo &SingerInfo::operator=(SingerInfo &&other) noexcept = default;

    SingerInfo::~SingerInfo() = default;

    QString SingerInfo::name() const {
        return d->name;
    }

    void SingerInfo::setName(const QString &name) {
        d->name = name;
    }

    QUrl SingerInfo::avatarUrl() const {
        return d->avatarUrl;
    }

    void SingerInfo::setAvatarUrl(const QUrl &avatarUrl) {
        d->avatarUrl = avatarUrl;
    }

    QUrl SingerInfo::backgroundUrl() const {
        return d->backgroundUrl;
    }

    void SingerInfo::setBackgroundUrl(const QUrl &backgroundUrl) {
        d->backgroundUrl = backgroundUrl;
    }

    QString SingerInfo::defaultLanguage() const {
        return d->defaultLanguage;
    }

    void SingerInfo::setDefaultLanguage(const QString &defaultLanguage) {
        d->defaultLanguage = defaultLanguage;
    }

    QStringList SingerInfo::languages() const {
        return d->languages;
    }

    void SingerInfo::setLanguages(const QStringList &languages) {
        d->languages = languages;
    }

    QString SingerInfo::mixGroup() const {
        return d->mixGroup;
    }

    void SingerInfo::setMixGroup(const QString &mixGroup) {
        d->mixGroup = mixGroup;
    }

    QVariant SingerInfo::userData() const {
        return d->userData;
    }

    void SingerInfo::setUserData(const QVariant &userData) {
        d->userData = userData;
    }

    QJsonValue SingerInfo::defaultExtra() const {
        return d->defaultExtra;
    }

    void SingerInfo::setDefaultExtra(const QJsonValue &defaultExtra) {
        d->defaultExtra = defaultExtra;
    }

    bool SingerInfo::operator==(const SingerInfo &other) const {
        return d.constData() == other.d.constData() ||
               (d->name == other.d->name && d->avatarUrl == other.d->avatarUrl &&
                d->backgroundUrl == other.d->backgroundUrl && d->defaultLanguage == other.d->defaultLanguage &&
                d->languages == other.d->languages && d->mixGroup == other.d->mixGroup &&
                d->userData == other.d->userData && d->defaultExtra == other.d->defaultExtra);
    }

    bool SingerInfo::operator!=(const SingerInfo &other) const {
        return !(*this == other);
    }

}
