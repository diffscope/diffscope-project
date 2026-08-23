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
        explicit EditorInteractionHelper(QObject *parent = nullptr);
        ~EditorInteractionHelper() override;

        static inline EditorInteractionHelper *create(QQmlEngine *, QJSEngine *) {
            return new EditorInteractionHelper;
        }

        static QStringList scrollModifierTexts();
        static QString shiftText();
        Q_INVOKABLE static QVariantList scrollBehaviorHints(int alternateAxisModifier, int zoomModifier, int pageModifier, bool usePageModifierAsAlternateAxisZoom, bool middleButtonAutoScroll);
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_EDITORINTERACTIONHELPER_H
