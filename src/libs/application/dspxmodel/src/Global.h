#ifndef DIFFSCOPE_DSPX_MODEL_GLOBAL_H
#define DIFFSCOPE_DSPX_MODEL_GLOBAL_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxmodel/DspxModelGlobal.h>

namespace QDspx {
    struct Global;
}

namespace dspx {

    class Model;

    class ModelPrivate;

    class GlobalPrivate;

    class DSPX_MODEL_EXPORT Global : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Global)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged)
        Q_PRIVATE_PROPERTY(d_func(), int centShift READ centShift WRITE setCentShift NOTIFY centShiftChanged)
        Q_PROPERTY(QString editorId READ editorId WRITE setEditorId NOTIFY editorIdChanged)
        Q_PROPERTY(QString editorName READ editorName WRITE setEditorName NOTIFY editorNameChanged)

    public:
        ~Global() override;

        QString name() const;
        void setName(const QString &name);

        QString author() const;
        void setAuthor(const QString &author);

        int centShift() const;
        void setCentShift(int centShift);

        QString editorId() const;
        void setEditorId(const QString &editorId);

        QString editorName() const;
        void setEditorName(const QString &editorName);

        QDspx::Global toQDspx() const;
        void fromQDspx(const QDspx::Global &global);

    Q_SIGNALS:
        void nameChanged(const QString &name);
        void authorChanged(const QString &author);
        void centShiftChanged(int centShift);
        void editorIdChanged(const QString &editorId);
        void editorNameChanged(const QString &editorName);

    private:
        friend class ModelPrivate;
        explicit Global(Model *model);

        QScopedPointer<GlobalPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_GLOBAL_H
