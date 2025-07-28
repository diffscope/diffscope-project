#ifndef DIFFSCOPE_COREPLUGIN_HOMEWINDOWDATA_H
#define DIFFSCOPE_COREPLUGIN_HOMEWINDOWDATA_H

#include <QObject>
#include <qqmlintegration.h>

#include <QAKQuick/quickactioncontext.h>

namespace Core::Internal {

    class HomeWindowData : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
    public:
        explicit HomeWindowData(QObject *parent = nullptr);
        ~HomeWindowData() override;
        QAK::QuickActionContext *actionContext() const;

    private:
        QAK::QuickActionContext *m_actionContext;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_HOMEWINDOWDATA_H
