// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_FILLLYRICSSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_FILLLYRICSSTRATEGY_H

#include <optional>

#include <QObject>
#include <QStringList>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class DSPXPROJECT_EXPORT FillLyricsStrategy : public QObject {
        Q_OBJECT
    public:
        explicit FillLyricsStrategy(QObject *parent = nullptr);
        ~FillLyricsStrategy() override;

        struct Result {
            QStringList lyrics;
            bool truncateToSelection;
        };

        virtual std::optional<Result> requestLyrics(const QStringList &initialLyrics, bool truncateToSelection,
                                                    QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_FILLLYRICSSTRATEGY_H
