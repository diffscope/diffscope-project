#ifndef DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_H
#define DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_H

#include <QObject>
#include <qqmlintegration.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {
    class TempoPropertyMapperPrivate;

    class TempoPropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(TempoPropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

    public:
        explicit TempoPropertyMapper(QObject *parent = nullptr);
        ~TempoPropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant pos() const;
        void setPos(const QVariant &pos);

        QVariant value() const;
        void setValue(const QVariant &value);

    Q_SIGNALS:
        void selectionModelChanged();
        void posChanged();
        void valueChanged();

    private:
        QScopedPointer<TempoPropertyMapperPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_TEMPOPROPERTYMAPPER_H
