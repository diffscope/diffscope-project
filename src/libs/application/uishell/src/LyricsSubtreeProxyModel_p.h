#ifndef UISHELL_LYRICSSUBTREEPROXYMODEL_P_H
#define UISHELL_LYRICSSUBTREEPROXYMODEL_P_H

#include "SubtreeProxyModel_p.h"
#include <qqmlintegration.h>

namespace UIShell {

    class LyricsSubtreeProxyModel : public SubtreeProxyModel {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit LyricsSubtreeProxyModel(QObject *parent = nullptr);
        ~LyricsSubtreeProxyModel() override;

        QHash<int, QByteArray> roleNames() const override;
    };

}

#endif //UISHELL_LYRICSSUBTREEPROXYMODEL_P_H
