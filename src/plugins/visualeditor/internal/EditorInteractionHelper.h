// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_EDITORINTERACTIONHELPER_H
#define DIFFSCOPE_VISUALEDITOR_EDITORINTERACTIONHELPER_H

#include <qqmlintegration.h>

#include <QObject>
#include <QStringList>
#include <QVariantList>

class QQmlEngine;
class QJSEngine;

namespace VisualEditor::Internal {

    class EditorInteractionHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(QStringList scrollModifierTexts READ scrollModifierTexts CONSTANT)
        Q_PROPERTY(QString shiftText READ shiftText CONSTANT)
    public:
        static EditorInteractionHelper *create(QQmlEngine *, QJSEngine *);

        QStringList scrollModifierTexts() const;
        QString shiftText() const;

        Q_INVOKABLE QVariantList scrollBehaviorHints(int alternateAxisModifier, int zoomModifier, int pageModifier, bool usePageModifierAsAlternateAxisZoom, bool middleButtonAutoScroll) const;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_EDITORINTERACTIONHELPER_H
