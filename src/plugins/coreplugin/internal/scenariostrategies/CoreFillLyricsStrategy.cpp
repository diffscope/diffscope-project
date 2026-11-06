// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreFillLyricsStrategy.h"

#include <QPointer>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/DocumentEditDialog.h>

namespace Core::Internal {

    CoreFillLyricsStrategy::CoreFillLyricsStrategy(DspxProject::Document *document) : m_document(document) {
    }

    namespace {

        const QString kCharacterExpression = QStringLiteral(R"(\s+|(?<=\S)(?=\S))");
        const QString kWordExpression = QStringLiteral(R"(\s+)");
        const QString kAutoExpression = [] {
            const QString baseCJK = QStringLiteral(R"([+\-あいうえお-ぢつ-もやゆよ-ろわ-ゔゕゖアイウエオ-ヂツ-モヤユヨ-ロワ-ヺ\p{Script=Han}\p{Script=Hangul}])");
            const QString smallKana = QStringLiteral(R"([ぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮ])");
            const QString western = QStringLiteral(R"([^\s+\-ぁ-ゖァ-ヺ\p{Script=Han}\p{Script=Hangul}])");
            const QString reTemplate = QStringLiteral(R"((?:\s+)|(?=%1)|(?<=%1)(?!%2)(?=\S)|(?<!%1)(?<!%2)(?=%2)|(?<=%2)(?=%3))");
            return reTemplate.arg(baseCJK, smallKana, western);
        }();

    }

    QString CoreFillLyricsStrategy::regularExpressionForSplitMode(DspxProject::FillLyricsScenario::SplitMode splitMode) {
        switch (splitMode) {
            case DspxProject::FillLyricsScenario::SplitMode_Auto:
                return kAutoExpression;
            case DspxProject::FillLyricsScenario::SplitMode_Character:
                return kCharacterExpression;
            case DspxProject::FillLyricsScenario::SplitMode_Word:
                return kWordExpression;
            case DspxProject::FillLyricsScenario::SplitMode_Regex:
                return {};
        }
        return {};
    }

    bool CoreFillLyricsStrategy::isRegularExpressionValid(const QString &pattern) {
        return QRegularExpression(pattern).isValid();
    }

    std::optional<DspxProject::FillLyricsStrategy::Result> CoreFillLyricsStrategy::requestLyrics(
        const QStringList &initialLyrics, bool truncateToSelection, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        const QPointer<QQuickWindow> dialogWindow(window);
        const QPointer<CoreFillLyricsStrategy> guard(this);
        auto state = RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.core.fillLyrics"));
        auto splitMode = static_cast<DspxProject::FillLyricsScenario::SplitMode>(
            state->value(QStringLiteral("splitMode"), DspxProject::FillLyricsScenario::SplitMode_Auto).toInt());
        auto regularExpression = state->value(QStringLiteral("regularExpression"), kAutoExpression).toString();
        state->endGroup();

        if (splitMode < DspxProject::FillLyricsScenario::SplitMode_Auto || splitMode > DspxProject::FillLyricsScenario::SplitMode_Regex) {
            splitMode = DspxProject::FillLyricsScenario::SplitMode_Auto;
        }
        if (splitMode != DspxProject::FillLyricsScenario::SplitMode_Regex) {
            regularExpression = regularExpressionForSplitMode(splitMode);
        }
        auto lyricsText = initialLyrics.join(QLatin1Char(' '));
        while (dialogWindow && guard) {
            auto dialog = DocumentEditDialog::createPopup("FillLyricsDialog", dialogWindow, DocumentEditDialog::placementFor(hints), {
                {"strategy", QVariant::fromValue(this)},
                {"noteCount", static_cast<int>(initialLyrics.size())},
                {"lyricsText", lyricsText},
                {"splitMode", static_cast<int>(splitMode)},
                {"regularExpression", regularExpression},
                {"truncateToSelection", truncateToSelection},
            });
            if (!DocumentEditDialog::execPopup(dialog.get(), dialogWindow) || !guard || !dialogWindow) {
                return std::nullopt;
            }

            lyricsText = dialog->property("lyricsText").toString();
            splitMode = static_cast<DspxProject::FillLyricsScenario::SplitMode>(dialog->property("splitMode").toInt());
            regularExpression = dialog->property("regularExpression").toString();
            truncateToSelection = dialog->property("truncateToSelection").toBool();
            const QRegularExpression expression(regularExpression);
            if (expression.isValid()) {
                state->beginGroup(QStringLiteral("org.diffscope.core.fillLyrics"));
                state->setValue(QStringLiteral("splitMode"), static_cast<int>(splitMode));
                state->setValue(QStringLiteral("regularExpression"), regularExpression);
                state->endGroup();
                return Result{splitLyrics(lyricsText, regularExpression), truncateToSelection};
            }

            SVS::MessageBox::critical(
                RuntimeInterface::qmlEngine(), dialogWindow,
                tr("Invalid Regular Expression"),
                tr("The regular expression is invalid:\n\n%1").arg(expression.errorString())
            );
        }
        return std::nullopt;
    }

    QStringList CoreFillLyricsStrategy::splitLyrics(const QString &lyrics, const QString &regularExpression) {
        return lyrics.split(QRegularExpression(regularExpression), Qt::SkipEmptyParts);
    }

    void CoreFillLyricsStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}

#include "moc_CoreFillLyricsStrategy.cpp"
