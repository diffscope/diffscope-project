#ifndef DIFFSCOPE_COREPLUGIN_DSPXINSPECTORDIALOG_P_H
#define DIFFSCOPE_COREPLUGIN_DSPXINSPECTORDIALOG_P_H

#include <coreplugin/DspxInspectorDialog.h>

class QTabWidget;
class QTreeView;
class QStandardItemModel;

namespace Core {

    class DspxInspectorDialogPrivate {
        Q_DECLARE_PUBLIC(DspxInspectorDialog)
    public:
        DspxInspectorDialog *q_ptr;

        QString path;
        QTabWidget *tabWidget;
        QTreeView *problemTreeView;

    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXINSPECTORDIALOG_P_H
