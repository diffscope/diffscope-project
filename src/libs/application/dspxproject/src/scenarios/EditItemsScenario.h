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

    class DSPXPROJECT_EXPORT EditItemsScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditItemsScenario)
    public:
        EditItemsScenario(Document *document, EditItemsStrategy *strategy, ParameterResamplingScenario *resampling, QObject *parent = nullptr);
        ~EditItemsScenario() override;

        Q_INVOKABLE void splitItems(int position) const;
        Q_INVOKABLE void bounceToClip(QQuickWindow *window) const;

        static QString splitTransactionName(const Document *document);

    private:
        QScopedPointer<EditItemsScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITITEMSSCENARIO_H
