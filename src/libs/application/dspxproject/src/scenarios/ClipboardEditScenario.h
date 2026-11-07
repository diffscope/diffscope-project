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

    /** Coordinates clipboard editing and owns the required edit transactions. */
    class DSPXPROJECT_EXPORT ClipboardEditScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ClipboardEditScenario)
        Q_PROPERTY(bool pasteAvailable READ pasteAvailable NOTIFY pasteAvailableChanged)
    public:
        /**
         * Creates a scenario with borrowed dependencies.
         * @pre document, strategy, clipboard, and resampling are non-null and valid during use.
         */
        ClipboardEditScenario(Document *document, ClipboardEditStrategy *strategy, Clipboard *clipboard, ParameterResamplingScenario *resampling, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~ClipboardEditScenario() override;

        /** Returns whether the clipboard advertises the current edit scope's payload type. */
        bool pasteAvailable() const;

        /** Copies the selection with a project playhead tick and returns whether copying succeeded. */
        Q_INVOKABLE bool copy(int playheadPosition) const;
        /** Copies the selection and deletes it in a transaction if copying succeeded. */
        Q_INVOKABLE void cut(int playheadPosition) const;
        /**
         * Pastes at the project playhead tick using mode and selects the pasted data.
         * Uses window as an optional dialog parent. Cancels if confirmation is declined
         * or the target becomes invalid during interaction.
         */
        Q_INVOKABLE void paste(int playheadPosition, Document::PasteMode mode, QQuickWindow *window) const;
        /** Deletes the selection in a transaction and returns whether data changed. */
        Q_INVOKABLE bool deleteSelection() const;

        /** Returns the translated transaction description for a payload type. */
        static QString pasteTransactionName(ClipboardData::Type type);
        /** Returns the translated deletion description. @pre document is non-null. */
        static QString deleteTransactionName(const Document *document);

    Q_SIGNALS:
        /** Notifies that paste availability changed. */
        void pasteAvailableChanged();

    private:
        QScopedPointer<ClipboardEditScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSCENARIO_H
