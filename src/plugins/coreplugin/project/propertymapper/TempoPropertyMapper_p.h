#ifndef DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_P_H
#define DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_P_H

#include "TempoPropertyMapper.h"

#include <QHash>
#include <QSet>
#include <QVariant>

namespace dspx {
    class Tempo;
    class TempoSelectionModel;
}

namespace Core {
    class TempoPropertyMapperPrivate {
        Q_DECLARE_PUBLIC(TempoPropertyMapper)
    public:
        explicit TempoPropertyMapperPrivate(TempoPropertyMapper *q);

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::TempoSelectionModel *tempoSelectionModel = nullptr;

        QHash<int, QSet<dspx::Tempo *>> posToTempos;
        QHash<double, QSet<dspx::Tempo *>> valueToTempos;
        QHash<dspx::Tempo *, int> tempoToPos;
        QHash<dspx::Tempo *, double> tempoToValue;

        QVariant cachedPos;
        QVariant cachedValue;
        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        void handleItemSelected(dspx::Tempo *tempo, bool selected);
        void addTempo(dspx::Tempo *tempo);
        void removeTempo(dspx::Tempo *tempo);
        void clear();

        void updatePos(dspx::Tempo *tempo, int pos);
        void updateValue(dspx::Tempo *tempo, double value);

        QVariant unifiedPos() const;
        QVariant unifiedValue() const;
        void refreshCache();

    private:
        TempoPropertyMapper *q_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_P_H
