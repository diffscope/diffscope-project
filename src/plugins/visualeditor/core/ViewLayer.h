// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_VIEWLAYER_H
#define DIFFSCOPE_VISUALEDITOR_VIEWLAYER_H

#include <QObject>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

namespace VisualEditor {

    class ViewLayerAttachedType;

    class VISUAL_EDITOR_EXPORT ViewLayer : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        QML_ATTACHED(ViewLayerAttachedType)

    public:
        enum ViewLayerPoint {
            PianoRollBackground,
            PianoRollContent,
            PianoRollForeground,
            PianoRollKeyboardForeground,
            ArrangementBackground,
            ArrangementContent,
            ArrangementForeground,
            ArrangementClipContent,
            ArrangementClipAdornment,
            ArrangementTimelineForeground,
            PianoRollTimelineForeground,
            PhonemeBackground,
            PhonemeForeground,
            ParameterBackground,
            ParameterForeground,
        };
        Q_ENUM(ViewLayerPoint)

        static ViewLayerAttachedType *qmlAttachedProperties(QObject *object);
    };

    using ViewLayerPoint = ViewLayer::ViewLayerPoint;

}

#endif // DIFFSCOPE_VISUALEDITOR_VIEWLAYER_H
