// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITSOURCESSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_EDITSOURCESSCENARIO_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace SingerCatalog {
    class SingerRegistry;
}

namespace dspx {
    class SingingClip;
}

namespace DspxProject {

    class Document;
    class EditSourcesStrategy;
    class EditSourcesScenarioPrivate;
    struct SourcesSelection;

    /**
     * Coordinates singing clip source changes and adjusts dynamic mixing ratios.
     * An empty architecture identifier or singer list removes the clip sources.
     * Window arguments are optional dialog parents. Empty clip lists perform no operation.
     * @pre Clip lists contain only non-null clips.
     */
    class DSPXPROJECT_EXPORT EditSourcesScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditSourcesScenario)

    public:
        /**
         * Creates a scenario with borrowed dependencies.
         * @pre document, strategy, and singerRegistry are non-null and valid during use.
         */
        EditSourcesScenario(Document *document, EditSourcesStrategy *strategy, const SingerCatalog::SingerRegistry *singerRegistry, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~EditSourcesScenario() override;

        /** Requests source selection and applies accepted changes to surviving clips in this document. */
        Q_INVOKABLE void editSources(QQuickWindow *window, const SourcesSelection &initial, const QList<dspx::SingingClip *> &clips, DocumentEdit::Hints hints = {});
        /** Applies sources in a transaction and reports unsupported note languages after changes. */
        Q_INVOKABLE void applySources(QQuickWindow *window, const SourcesSelection &sources, const QList<dspx::SingingClip *> &clips);

    private:
        QScopedPointer<EditSourcesScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITSOURCESSCENARIO_H
