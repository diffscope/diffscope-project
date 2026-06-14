#include "FillLyricsAddOn.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <QEventLoop>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QQmlComponent>
#include <QSettings>
#include <QStringList>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/MessageBox.h>

#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/SelectionModel.h>

#include <transactional/TransactionController.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

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

        bool execDialog(QObject *dialog) {
            QEventLoop eventLoop;
            QObject::connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
            QObject::connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
            QMetaObject::invokeMethod(dialog, "open");
            eventLoop.exec();
            return dialog->property("result").toInt() == 1;
        }

    }

    FillLyricsAddOn::FillLyricsAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    FillLyricsAddOn::~FillLyricsAddOn() = default;

    void FillLyricsAddOn::initialize() {
        auto *windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "FillLyricsAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto *actions = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        actions->setParent(this);
        QMetaObject::invokeMethod(actions, "registerToContext", windowInterface->actionContext());
    }
    void FillLyricsAddOn::extensionsInitialized() {
    }
    bool FillLyricsAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    QString FillLyricsAddOn::regularExpressionForSplitMode(SplitMode splitMode) {
        switch (splitMode) {
            case SplitMode_Auto:
                return kAutoExpression;
            case SplitMode_Character:
                return kCharacterExpression;
            case SplitMode_Word:
                return kWordExpression;
            case SplitMode_Regex:
                return {};
        }
        return {};
    }

    bool FillLyricsAddOn::isRegularExpressionValid(const QString &pattern) {
        return QRegularExpression(pattern).isValid();
    }

    void FillLyricsAddOn::fillLyrics() {
        auto *windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto *window = qobject_cast<QQuickWindow *>(windowInterface->window());
        auto *document = windowInterface->projectDocumentContext()->document();
        auto *selectionModel = document->selectionModel();
        auto *noteSelectionModel = selectionModel->noteSelectionModel();
        if (!window || selectionModel->selectionType() != dspx::SelectionModel::ST_Note || noteSelectionModel->selectedCount() == 0) {
            return;
        }

        auto *settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        auto splitMode = static_cast<SplitMode>(settings->value(QStringLiteral("splitMode"), SplitMode_Auto).toInt());
        auto regularExpression = settings->value(QStringLiteral("regularExpression"), kAutoExpression).toString();
        settings->endGroup();

        if (splitMode < SplitMode_Auto || splitMode > SplitMode_Regex) {
            splitMode = SplitMode_Auto;
        }
        if (splitMode != SplitMode_Regex) {
            regularExpression = regularExpressionForSplitMode(splitMode);
        }

        auto notes = noteSelectionModel->selectedItems();
        std::sort(notes.begin(), notes.end(), [](const dspx::Note *lhs, const dspx::Note *rhs) {
            return lhs->pos() < rhs->pos();
        });

        QStringList initialLyrics;
        initialLyrics.reserve(notes.size());
        for (const auto *note : std::as_const(notes)) {
            initialLyrics.append(note->lyric());
        }
        QString lyricsText = initialLyrics.join(QLatin1Char(' '));
        bool truncateToSelection = noteSelectionModel->selectedCount() != 1;

        while (true) {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "FillLyricsDialog");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            std::unique_ptr<QObject> dialog(component.createWithInitialProperties({
                {"parent", QVariant::fromValue(window->contentItem())},
                {"addOn", QVariant::fromValue(this)},
                {"lyricsText", lyricsText},
                {"splitMode", splitMode},
                {"regularExpression", regularExpression},
                {"truncateToSelection", truncateToSelection},
            }));
            if (!dialog) {
                qFatal() << component.errorString();
            }

            dialog->setProperty("x", window->width() / 2.0 - dialog->property("width").toDouble() / 2.0);
            if (const auto topMargin = window->property("popupTopMarginHint"); topMargin.isValid()) {
                dialog->setProperty("y", topMargin);
            } else {
                dialog->setProperty("y", window->height() / 2.0 - dialog->property("height").toDouble() / 2.0);
            }

            if (!execDialog(dialog.get())) {
                return;
            }

            lyricsText = dialog->property("lyricsText").toString();
            splitMode = static_cast<SplitMode>(dialog->property("splitMode").toInt());
            regularExpression = dialog->property("regularExpression").toString();
            truncateToSelection = dialog->property("truncateToSelection").toBool();

            const QRegularExpression expression(regularExpression);
            if (expression.isValid()) {
                break;
            }

            SVS::MessageBox::critical(
                RuntimeInterface::qmlEngine(),
                window,
                tr("Invalid Regular Expression"),
                tr("The regular expression is invalid:\n\n%1").arg(expression.errorString())
            );
        }

        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("splitMode"), splitMode);
        settings->setValue(QStringLiteral("regularExpression"), regularExpression);
        settings->endGroup();

        auto lyricList = splitLyrics(lyricsText, regularExpression);
        if (lyricList.isEmpty()) {
            return;
        }

        if (notes.isEmpty()) {
            return;
        }

        if (!truncateToSelection && notes.size() < lyricList.size()) {
            auto *nextNote = notes.constLast()->nextItem();
            while (nextNote && notes.size() < lyricList.size()) {
                auto *currentNote = nextNote;
                nextNote = currentNote->nextItem();
                if (noteSelectionModel->isItemSelected(currentNote)) {
                    continue;
                }
                selectionModel->select(currentNote, dspx::SelectionModel::Select);
                notes.append(currentNote);
            }
        }

        document->transactionController()->beginScopedTransaction(tr("Filling lyrics"), [notes, lyricList] {
            const auto count = std::min(notes.size(), lyricList.size());
            for (qsizetype i = 0; i < count; ++i) {
                notes.at(i)->setLyric(lyricList.at(i));
            }
            return count > 0;
        });
    }

    QStringList FillLyricsAddOn::splitLyrics(const QString &lyrics, const QString &regularExpression) {
        return lyrics.split(QRegularExpression(regularExpression), Qt::SkipEmptyParts);
    }

}
