// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class Document;
    class FillLyricsScenarioPrivate;
    class FillLyricsStrategy;

    class DSPXPROJECT_EXPORT FillLyricsScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(FillLyricsScenario)
    public:
        enum SplitMode {
            SplitMode_Auto,
            SplitMode_Character,
            SplitMode_Word,
            SplitMode_Regex,
        };
        Q_ENUM(SplitMode)

        FillLyricsScenario(Document *document, FillLyricsStrategy *strategy, QObject *parent = nullptr);
        ~FillLyricsScenario() override;

        Q_INVOKABLE void fillLyrics(QQuickWindow *window, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<FillLyricsScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_H
