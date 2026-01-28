#ifndef DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_P_H
#define DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_P_H

#include "LabelPropertyMapper.h"

#include <QHash>
#include <QSet>
#include <QVariant>

namespace dspx {
    class Label;
    class LabelSelectionModel;
}

namespace Core {
    class LabelPropertyMapperPrivate {
        Q_DECLARE_PUBLIC(LabelPropertyMapper)
    public:
        explicit LabelPropertyMapperPrivate(LabelPropertyMapper *q);

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::LabelSelectionModel *labelSelectionModel = nullptr;

        QHash<int, QSet<dspx::Label *>> posToLabels;
        QHash<QString, QSet<dspx::Label *>> textToLabels;
        QHash<dspx::Label *, int> labelToPos;
        QHash<dspx::Label *, QString> labelToText;

        QVariant cachedPos;
        QVariant cachedText;
        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();

        void handleItemSelected(dspx::Label *label, bool selected);
        void addLabel(dspx::Label *label);
        void removeLabel(dspx::Label *label);
        void clear();

        void updatePos(dspx::Label *label, int pos);
        void updateText(dspx::Label *label, const QString &text);

        QVariant unifiedPos() const;
        QVariant unifiedText() const;
        void refreshCache();

    private:
        LabelPropertyMapper *q_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_P_H
