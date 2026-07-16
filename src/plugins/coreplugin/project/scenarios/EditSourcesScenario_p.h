#ifndef DIFFSCOPE_COREPLUGIN_EDITSOURCESSCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_EDITSOURCESSCENARIO_P_H

#include <QMetaObject>
#include <QPointer>

#include <coreplugin/EditSourcesScenario.h>

namespace Core {

    class EditSourcesScenarioPrivate {
        Q_DECLARE_PUBLIC(EditSourcesScenario)

    public:
        EditSourcesScenario *q_ptr{};
        QPointer<QQuickWindow> window;
        QPointer<DspxDocument> document;
        QMetaObject::Connection windowDestroyedConnection;
        QMetaObject::Connection documentDestroyedConnection;
        bool dialogAccepted{};

        bool execDialog(SourcesPickerModel *model);
        bool applySourcesImpl(SourcesPickerModel *model,
                              const QList<QPointer<dspx::SingingClip>> &clips) const;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_EDITSOURCESSCENARIO_P_H
