#ifndef DIFFSCOPE_COREPLUGIN_DYNAMICMIXINGANCHORPROPERTYMAPPER_P_H
#define DIFFSCOPE_COREPLUGIN_DYNAMICMIXINGANCHORPROPERTYMAPPER_P_H

#include <coreplugin/DynamicMixingAnchorPropertyMapper.h>

#include <QList>
#include <QMetaObject>

namespace dspx {
    class DynamicMixingAnchor;
    class DynamicMixingAnchorSequence;
}

namespace Core {

    class DynamicMixingAnchorPropertyMapperPrivate {
        Q_DECLARE_PUBLIC(DynamicMixingAnchorPropertyMapper)
    public:
        DynamicMixingAnchorPropertyMapper *q_ptr = nullptr;
        dspx::SelectionModel *selectionModel = nullptr;
        dspx::DynamicMixingAnchorSequence *sequence = nullptr;
        QList<QMetaObject::Connection> connections;

        void disconnectAll();
        void refreshConnections();
        QList<dspx::DynamicMixingAnchor *> selectedItems() const;
        QList<double> effectiveRatios(const dspx::DynamicMixingAnchor *item) const;
        void notifyAll();
    };

}

#endif // DIFFSCOPE_COREPLUGIN_DYNAMICMIXINGANCHORPROPERTYMAPPER_P_H
