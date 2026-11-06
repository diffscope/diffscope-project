// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREFILLLYRICSSTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREFILLLYRICSSTRATEGY_H

#include <dspxproject/FillLyricsScenario.h>
#include <dspxproject/FillLyricsStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CoreFillLyricsStrategy : public DspxProject::FillLyricsStrategy {
        Q_OBJECT
    public:
        explicit CoreFillLyricsStrategy(DspxProject::Document *document);

        std::optional<Result> requestLyrics(const QStringList &initialLyrics, bool truncateToSelection,
                                            QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;

        Q_INVOKABLE static QString regularExpressionForSplitMode(DspxProject::FillLyricsScenario::SplitMode splitMode);
        Q_INVOKABLE static bool isRegularExpressionValid(const QString &pattern);
        Q_INVOKABLE static QStringList splitLyrics(const QString &lyrics, const QString &regularExpression);
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREFILLLYRICSSTRATEGY_H
