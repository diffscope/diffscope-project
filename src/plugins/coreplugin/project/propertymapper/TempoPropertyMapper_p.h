#ifndef DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_P_H
#define DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_P_H

#include "TempoPropertyMapper.h"

#include <dspxmodel/Tempo.h>

#include <CorePlugin/private/PropertyMapperData_p.h>

namespace dspx {
    class TempoSelectionModel;
}

namespace Core {
    class TempoPropertyMapperPrivate : public PropertyMapperData<
        TempoPropertyMapper,
        TempoPropertyMapperPrivate,
        dspx::Tempo,
        PropertyMetadata<dspx::Tempo, int, int>, // pos
        PropertyMetadata<dspx::Tempo, double, double> // value
    > {
        Q_DECLARE_PUBLIC(TempoPropertyMapper)
    public:
        TempoPropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Tempo::pos, &dspx::Tempo::setPos, &dspx::Tempo::posChanged},
            {&dspx::Tempo::value, &dspx::Tempo::setValue, &dspx::Tempo::valueChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::TempoSelectionModel *tempoSelectionModel = nullptr;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        enum {
            PosProperty = 0,
            ValueProperty = 1
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(TempoPropertyMapper);
            if constexpr (i == PosProperty) {
                q->posChanged();
            } else if constexpr (i == ValueProperty) {
                q->valueChanged();
            }
        }
    };
}

#endif // DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_P_H
