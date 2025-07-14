#ifndef UISHELL_USDEFNAMESPACE_H
#define UISHELL_USDEFNAMESPACE_H

#include <QMetaObject>

namespace UIShell {

    namespace USDef {
        Q_NAMESPACE

        enum LyricCellRole {
            PronunciationRole = Qt::UserRole + 1,
            LyricRole,
            CandidatePronunciationsRole,
        };
        Q_ENUM_NS(LyricCellRole)

    }

}

#endif //UISHELL_USDEFNAMESPACE_H
