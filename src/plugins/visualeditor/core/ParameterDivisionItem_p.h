#ifndef DIFFSCOPE_VISUALEDITOR_PARAMETERDIVISIONITEM_P_H
#define DIFFSCOPE_VISUALEDITOR_PARAMETERDIVISIONITEM_P_H

#include <visualeditor/ParameterDivisionItem.h>

namespace VisualEditor {
    class ParameterDivisionItemPrivate {
        Q_DECLARE_PUBLIC(ParameterDivisionItem)

    public:
        ParameterDivisionItem *q_ptr{};
        Core::ParameterInfo parameterInfo;
        QColor color;
        qreal lineLength{6.0};
    };
}

#endif // DIFFSCOPE_VISUALEDITOR_PARAMETERDIVISIONITEM_P_H
