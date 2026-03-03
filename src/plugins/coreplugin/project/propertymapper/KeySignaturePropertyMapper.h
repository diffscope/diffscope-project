#ifndef DIFFSCOPE_COREPLUGIN_KEYSIGNATUREPROPERTYMAPPER_H
#define DIFFSCOPE_COREPLUGIN_KEYSIGNATUREPROPERTYMAPPER_H

#include <QObject>
#include <qqmlintegration.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {
    class KeySignaturePropertyMapperPrivate;

    class KeySignaturePropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(KeySignaturePropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(QVariant mode READ mode WRITE setMode NOTIFY modeChanged)
        Q_PROPERTY(QVariant tonality READ tonality WRITE setTonality NOTIFY tonalityChanged)
        Q_PROPERTY(QVariant accidentalType READ accidentalType WRITE setAccidentalType NOTIFY accidentalTypeChanged)

    public:
        explicit KeySignaturePropertyMapper(QObject *parent = nullptr);
        ~KeySignaturePropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant pos() const;
        void setPos(const QVariant &pos);

        QVariant mode() const;
        void setMode(const QVariant &mode);

        QVariant tonality() const;
        void setTonality(const QVariant &tonality);

        QVariant accidentalType() const;
        void setAccidentalType(const QVariant &accidentalType);

    Q_SIGNALS:
        void selectionModelChanged();
        void posChanged();
        void modeChanged();
        void tonalityChanged();
        void accidentalTypeChanged();

    private:
        QScopedPointer<KeySignaturePropertyMapperPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_KEYSIGNATUREPROPERTYMAPPER_H
