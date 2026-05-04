#ifndef UISHELL_USDEFNAMESPACE_H
#define UISHELL_USDEFNAMESPACE_H

#include <QMetaObject>

namespace UIShell {

    namespace USDef {
        Q_NAMESPACE

        enum LyricCellRole {
            LC_PronunciationRole = Qt::UserRole + 1,
            LC_LyricRole,
            LC_CandidatePronunciationsRole,
        };
        Q_ENUM_NS(LyricCellRole)

        enum RecentFileRole {
            RF_NameRole = Qt::UserRole + 1,
            RF_PathRole,
            RF_LastModifiedTextRole,
            RF_ThumbnailRole,
            RF_IconRole,
            RF_ColorizeRole,
        };
        Q_ENUM_NS(RecentFileRole)

        enum AchievementRole {
            AR_IdRole = Qt::UserRole + 1,
            AR_NameRole,
            AR_DescriptionRole,
            AR_IconRole,
            AR_IconColorRole,
            AR_HiddenRole,
        };
        Q_ENUM_NS(AchievementRole)

        enum PackageRole {
            PR_IdRole = Qt::UserRole + 1,
            PR_VersionRole,
            PR_PathRole,
            PR_InstallationTimeRole,
            PR_NameRole,
            PR_DescriptionRole,
            PR_VendorRole,
            PR_ReadmePathRole,
            PR_LicensePathRole,
            PR_UrlRole,
            PR_ClassNameRole,
            PR_AvatarPathRole,
            PR_BackgroundPathRole,
            PR_ImportInferenceIdRole,
        };
        Q_ENUM_NS(PackageRole)

        enum PackageIndex {
            PI_Dependencies = 0,
            PI_Inferences = 1,
            PI_Singers = 2,

            PI_SingerImports = 0,
            PI_SingerDemoAudioList = 1,
        };
        Q_ENUM_NS(PackageIndex)

    }

}

#endif //UISHELL_USDEFNAMESPACE_H
