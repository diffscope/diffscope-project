// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/Document.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class ClipboardEditScenarioPrivate;
    class ClipboardEditStrategy;
    class Clipboard;
    class ParameterResamplingScenario;

    class DSPXPROJECT_EXPORT ClipboardEditScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ClipboardEditScenario)
        Q_PROPERTY(bool pasteAvailable READ pasteAvailable NOTIFY pasteAvailableChanged)
    public:
        ClipboardEditScenario(Document *document, ClipboardEditStrategy *strategy, Clipboard *clipboard, ParameterResamplingScenario *resampling, QObject *parent = nullptr);
        ~ClipboardEditScenario() override;

        bool pasteAvailable() const;

        Q_INVOKABLE bool copy(int playheadPosition) const;
        Q_INVOKABLE void cut(int playheadPosition) const;
        Q_INVOKABLE void paste(int playheadPosition, Document::PasteMode mode, QQuickWindow *window) const;
        Q_INVOKABLE bool deleteSelection() const;

        static QString pasteTransactionName(ClipboardData::Type type);
        static QString deleteTransactionName(const Document *document);

    Q_SIGNALS:
        void pasteAvailableChanged();

    private:
        QScopedPointer<ClipboardEditScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSCENARIO_H
