#ifndef DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_H
#define DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_H

#include <QObject>
#include <qqmlintegration.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {
    class LabelPropertyMapperPrivate;

    class LabelPropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(LabelPropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(QVariant text READ text WRITE setText NOTIFY textChanged)

    public:
        explicit LabelPropertyMapper(QObject *parent = nullptr);
        ~LabelPropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant pos() const;
        void setPos(const QVariant &pos);

        QVariant text() const;
        void setText(const QVariant &text);

    Q_SIGNALS:
        void selectionModelChanged();
        void posChanged();
        void textChanged();

    private:
        QScopedPointer<LabelPropertyMapperPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_LABELPROPERTYMAPPER_H
