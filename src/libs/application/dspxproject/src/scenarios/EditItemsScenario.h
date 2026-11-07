// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITITEMSSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_EDITITEMSSCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class Document;
    class EditItemsScenarioPrivate;
    class EditItemsStrategy;
    class ParameterResamplingScenario;

    /** Coordinates splitting and bouncing with edit transactions and risk confirmation. */
    class DSPXPROJECT_EXPORT EditItemsScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditItemsScenario)
    public:
        /**
         * Creates a scenario with borrowed dependencies.
         * @pre document, strategy, and resampling are non-null and valid during use.
         */
        EditItemsScenario(Document *document, EditItemsStrategy *strategy, ParameterResamplingScenario *resampling, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~EditItemsScenario() override;

        /**
         * Splits the selection in a transaction and selects the inserted right parts.
         * position uses project ticks for clips and clip content ticks for notes.
         */
        Q_INVOKABLE void splitItems(int position) const;
        /** Bounces selected singing clips after risk confirmation with an optional dialog window. */
        Q_INVOKABLE void bounceToClip(QQuickWindow *window) const;

        /**
         * Returns the translated split description, or an empty string for an unsupported scope.
         * @pre document is non-null.
         */
        static QString splitTransactionName(const Document *document);

    private:
        QScopedPointer<EditItemsScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITITEMSSCENARIO_H
