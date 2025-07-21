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
        };
        Q_ENUM_NS(RecentFileRole)

    }

}

#endif //UISHELL_USDEFNAMESPACE_H
