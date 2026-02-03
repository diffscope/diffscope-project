#ifndef DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_P_H
#define DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_P_H

#include "LabelPropertyMapper.h"

#include <dspxmodel/Label.h>

#include <CorePlugin/private/PropertyMapperData_p.h>

namespace dspx {
    class LabelSelectionModel;
}

namespace Core {
    class LabelPropertyMapperPrivate : public PropertyMapperData<
        LabelPropertyMapper,
        LabelPropertyMapperPrivate,
        dspx::Label,
        PropertyMetadata<dspx::Label, int, int>, // pos
        PropertyMetadata<dspx::Label, QString, const QString &> // text
    > {
        Q_DECLARE_PUBLIC(LabelPropertyMapper)
    public:
        LabelPropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Label::pos, &dspx::Label::setPos, &dspx::Label::posChanged},
            {&dspx::Label::text, &dspx::Label::setText, &dspx::Label::textChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::LabelSelectionModel *labelSelectionModel = nullptr;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        enum {
            PosProperty = 0,
            TextProperty = 1
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(LabelPropertyMapper);
            if constexpr (i == PosProperty) {
                q->posChanged();
            } else if constexpr (i == TextProperty) {
                q->textChanged();
            }
        }
    };
}

#endif // DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_P_H
