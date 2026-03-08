#ifndef DIFFSCOPE_COREPLUGIN_DSPXINSPECTORDIALOG_H
#define DIFFSCOPE_COREPLUGIN_DSPXINSPECTORDIALOG_H

#include <QDialog>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace Core {

    class DspxInspectorDialogPrivate;

    class CORE_EXPORT DspxInspectorDialog : public QDialog {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(DspxInspectorDialog)
        Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    public:
        explicit DspxInspectorDialog(QWidget *parent = nullptr);
        ~DspxInspectorDialog() override;

        QString path() const;
        void setPath(const QString &path);

    public slots:
        void runCheck();

    Q_SIGNALS:
        void pathChanged(const QString &path);

    private:
        QScopedPointer<DspxInspectorDialogPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXINSPECTORDIALOG_H
