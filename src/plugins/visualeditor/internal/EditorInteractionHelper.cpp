// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditorInteractionHelper.h"

#include <QKeySequence>
#include <QVariantMap>

#include <ScopicFlowCore/ScrollBehaviorViewModel.h>

namespace VisualEditor::Internal {

    EditorInteractionHelper *EditorInteractionHelper::create(QQmlEngine *, QJSEngine *) {
        return new EditorInteractionHelper;
    }

    QStringList EditorInteractionHelper::scrollModifierTexts() const {
        return {
            QKeySequence(Qt::ControlModifier).toString(QKeySequence::NativeText) + tr("Scroll"),
            QKeySequence(Qt::AltModifier).toString(QKeySequence::NativeText) + tr("Scroll"),
            QKeySequence(Qt::ShiftModifier).toString(QKeySequence::NativeText) + tr("Scroll"),
        };
    }

    QString EditorInteractionHelper::shiftText() const {
        return QKeySequence(Qt::Key_Shift).toString(QKeySequence::NativeText);
    }

    QVariantList EditorInteractionHelper::scrollBehaviorHints(int alternateAxisModifier, int zoomModifier, int pageModifier, bool usePageModifierAsAlternateAxisZoom, bool middleButtonAutoScroll) const {
        static const Qt::KeyboardModifier scrollModifierValues[] = {Qt::ControlModifier, Qt::AltModifier, Qt::ShiftModifier};
        auto modifierValue = [](int scrollModifier) {
            return scrollModifierValues[qBound(0, scrollModifier, 2)];
        };
        sflow::ScrollBehaviorViewModel scrollBehaviorViewModel;
        scrollBehaviorViewModel.setAlternateAxisModifier(modifierValue(alternateAxisModifier));
        scrollBehaviorViewModel.setZoomModifier(modifierValue(zoomModifier));
        scrollBehaviorViewModel.setPageModifier(modifierValue(pageModifier));
        scrollBehaviorViewModel.setUsePageModifierAsAlternateAxisZoom(usePageModifierAsAlternateAxisZoom);

        auto modifierText = [](Qt::KeyboardModifiers modifiers) {
            // Note that QKeySequence::toString(QKeySequence::NativeText) of a single modifier ends with a '+'.
            QString text;
            if (modifiers & Qt::ControlModifier)
                text += QKeySequence(Qt::ControlModifier).toString(QKeySequence::NativeText);
            if (modifiers & Qt::AltModifier)
                text += QKeySequence(Qt::AltModifier).toString(QKeySequence::NativeText);
            if (modifiers & Qt::ShiftModifier)
                text += QKeySequence(Qt::ShiftModifier).toString(QKeySequence::NativeText);
            return text;
        };

        static const char *behaviorTexts[] = {
            QT_TR_NOOP("Vertical move"),
            QT_TR_NOOP("Horizontal move"),
            QT_TR_NOOP("Vertical move by page"),
            QT_TR_NOOP("Horizontal move by page"),
            QT_TR_NOOP("Vertical zoom"),
            QT_TR_NOOP("Horizontal zoom"),
            QT_TR_NOOP("Quick vertical zoom"),
            QT_TR_NOOP("Quick horizontal zoom"),
        };

        const Qt::KeyboardModifier alternateAxis = modifierValue(alternateAxisModifier);
        const Qt::KeyboardModifier zoom = modifierValue(zoomModifier);
        const Qt::KeyboardModifier page = modifierValue(pageModifier);
        const Qt::KeyboardModifiers combinations[] = {
            Qt::KeyboardModifiers{},
            alternateAxis,
            zoom,
            page,
            alternateAxis | zoom,
            alternateAxis | page,
            zoom | page,
            alternateAxis | zoom | page,
        };
        QVariantList hints;
        for (auto modifiers : combinations) {
            int behavior = 0;
            if (scrollBehaviorViewModel.isZoom(modifiers))
                behavior |= 4;
            if (scrollBehaviorViewModel.isPage(modifiers))
                behavior |= 2;
            // Do not use ScrollBehaviorViewModel::isAlternateAxis() here: it contains a
            // platform-specific event-axis adjustment (FIXME), while the hints present the
            // configured semantics — holding the alternate-axis modifier (or the page
            // modifier in horizontal zoom mode) means horizontal.
            if ((modifiers & alternateAxis) || (usePageModifierAsAlternateAxisZoom && (modifiers & page)))
                behavior |= 1;
            hints.append(QVariantMap{
                {QStringLiteral("combination"), modifierText(modifiers) + tr("Wheel")},
                {QStringLiteral("behavior"), tr(behaviorTexts[behavior])},
            });
        }

        // Middle button behaviors in StandardScrollHandler: pressing the zoom modifier
        // makes dragging zoom the view; otherwise Alt inverts the auto scroll mode.
        const QString middleButtonDragText = tr("Middle button drag");
        hints.append(QVariantMap{
            {QStringLiteral("combination"), middleButtonDragText},
            {QStringLiteral("behavior"), middleButtonAutoScroll ? tr("Auto-scrolling move") : tr("Dragging move")},
        });
        hints.append(QVariantMap{
            {QStringLiteral("combination"), QKeySequence(Qt::AltModifier).toString(QKeySequence::NativeText) + middleButtonDragText},
            {QStringLiteral("behavior"), middleButtonAutoScroll ? tr("Dragging move") : tr("Auto-scrolling move")},
        });
        hints.append(QVariantMap{
            {QStringLiteral("combination"), QKeySequence(modifierValue(zoomModifier)).toString(QKeySequence::NativeText) + middleButtonDragText},
            {QStringLiteral("behavior"), tr("Zoom")},
        });
        return hints;
    }

}
