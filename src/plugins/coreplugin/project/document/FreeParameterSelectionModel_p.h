#ifndef DIFFSCOPE_COREPLUGIN_FREEPARAMETERSELECTIONMODEL_P_H
#define DIFFSCOPE_COREPLUGIN_FREEPARAMETERSELECTIONMODEL_P_H

#include <QPointer>
#include <QMetaObject>

#include <coreplugin/FreeParameterSelectionModel.h>

namespace Core {
    class FreeParameterSelectionModelPrivate {
        Q_DECLARE_PUBLIC(FreeParameterSelectionModel)

    public:
        FreeParameterSelectionModel *q_ptr{};
        dspx::SelectionModel *selectionModel{};
        QPointer<dspx::SingingClip> singingClip;
        QString parameterId;
        QString displayName;
        FreeParameterSelectionModel::Layer layer{FreeParameterSelectionModel::EditedLayer};
        int start{};
        int end{};
        bool hasSelection{};
        QMetaObject::Connection singingClipDestroyedConnection;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_FREEPARAMETERSELECTIONMODEL_P_H
