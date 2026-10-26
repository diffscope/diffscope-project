// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_PACKAGEFILEOPENHANDLER_H
#define DIFFSCOPE_SYNTH_INTERNAL_PACKAGEFILEOPENHANDLER_H

#include <coreplugin/IFileOpenHandler.h>

namespace Synth::Internal {

    class PackageManagerController;

    class PackageFileOpenHandler : public Core::IFileOpenHandler {
        Q_OBJECT
    public:
        explicit PackageFileOpenHandler(PackageManagerController *packageManager, QObject *parent = nullptr);
        ~PackageFileOpenHandler() override;

        QSet<qsizetype> preflight(const QStringList &filePaths, QWindow *parentWindow) const override;
        QSet<qsizetype> openFiles(const QStringList &filePaths, QWindow *parentWindow) override;

    private:
        PackageManagerController *m_packageManager;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_PACKAGEFILEOPENHANDLER_H
