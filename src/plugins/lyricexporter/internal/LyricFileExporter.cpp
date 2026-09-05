// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "LyricFileExporter.h"

#include <memory>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QStringList>
#include <QVariant>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <lyricexporter/internal/LyricExportFormat.h>
#include <lyricexporter/internal/LyricExportSession.h>

namespace LyricExporter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcLyricFileExporter, "diffscope.lyricexporter.fileexporter")

    static const LyricExportFormat *formatFromPath(const QString &path) {
        // TODO: Use the selected file dialog filter index after FileConverter exposes it.
        const QString suffix = QFileInfo(path).suffix().toLower();
        for (const auto &format : lyricExportFormats()) {
            if (suffix == QLatin1StringView(format.suffix))
                return &format;
        }
        return nullptr;
    }

    LyricFileExporter::LyricFileExporter(QObject *parent) : FileConverter(parent) {
        setName(tr("Lyrics"));
        setDescription(tr("Export lyrics as timed text (such as LRC, SubRip, etc.) or plain text files"));

        QStringList fileDialogFilters;
        QStringList heuristicFilters;
        fileDialogFilters.reserve(static_cast<qsizetype>(lyricExportFormats().size()));
        heuristicFilters.reserve(static_cast<qsizetype>(lyricExportFormats().size()));
        for (const auto &format : lyricExportFormats()) {
            fileDialogFilters.append(QCoreApplication::translate(
                "LyricExporter::Internal::LyricFileExporter", format.fileDialogFilter
            ));
            heuristicFilters.append(QString::fromLatin1(format.heuristicFilter));
        }
        setFileDialogFilters(fileDialogFilters);
        setMode(Export);
        setHeuristicFilters(heuristicFilters);
    }

    LyricFileExporter::~LyricFileExporter() = default;

    bool LyricFileExporter::execExport(
        const QString &path, const opendspx::Model &model, QWindow *window
    ) {
        const auto format = formatFromPath(path);
        if (!format) {
            qCWarning(lcLyricFileExporter) << "Unsupported output suffix:" << path;
            SVS::MessageBox::critical(
                Core::RuntimeInterface::qmlEngine(), window,
                tr("Unsupported Lyric Format"),
                tr("Choose a file name ending in the extension of supported formats.")
            );
            return false;
        }

        LyricExportSession session(*format, path, model, window);
        QQmlComponent component(
            Core::RuntimeInterface::qmlEngine(), "DiffScope.LyricExporter", "LyricExportDialog"
        );
        if (component.isError()) {
            qFatal() << component.errorString();
            return false;
        }

        QObject *object = component.createWithInitialProperties({
            {QStringLiteral("session"), QVariant::fromValue(&session)},
        });
        QWindow *dialogWindow = qobject_cast<QWindow *>(object);
        if (!dialogWindow) {
            delete object;
            const QString detail = component.errorString().isEmpty()
                ? tr("Could not create the lyric export window.")
                : component.errorString();
            qFatal() << detail;
            return false;
        }
        std::unique_ptr<QWindow> dialog(dialogWindow);

        dialog->setTransientParent(window);
        session.setDialogWindow(dialog.get());
        dialog->show();

        QEventLoop eventLoop;
        connect(dialog.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();
        return session.exported();
    }

}
