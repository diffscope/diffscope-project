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

    class DSPXPROJECT_EXPORT EditSourcesScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditSourcesScenario)

    public:
        EditSourcesScenario(Document *document, EditSourcesStrategy *strategy, const SingerCatalog::SingerRegistry *singerRegistry, QObject *parent = nullptr);
        ~EditSourcesScenario() override;

        Q_INVOKABLE void editSources(QQuickWindow *window, const SourcesSelection &initial, const QList<dspx::SingingClip *> &clips, DocumentEdit::Hints hints = {});
        Q_INVOKABLE void applySources(QQuickWindow *window, const SourcesSelection &sources, const QList<dspx::SingingClip *> &clips);

    private:
        QScopedPointer<EditSourcesScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITSOURCESSCENARIO_H
