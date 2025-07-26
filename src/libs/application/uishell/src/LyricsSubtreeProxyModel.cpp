#include "LyricsSubtreeProxyModel_p.h"
#include <uishell/USDef.h>

namespace UIShell {

    LyricsSubtreeProxyModel::LyricsSubtreeProxyModel(QObject *parent)
        : SubtreeProxyModel(parent) {}

    LyricsSubtreeProxyModel::~LyricsSubtreeProxyModel() = default;

    QHash<int, QByteArray> LyricsSubtreeProxyModel::roleNames() const {
        return {
            {USDef::LC_PronunciationRole, "pronunciation"},
            {USDef::LC_LyricRole, "lyric"},
            {USDef::LC_CandidatePronunciationsRole, "candidatePronunciations"}
        };
    }

}
