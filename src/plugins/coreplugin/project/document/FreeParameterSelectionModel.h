#ifndef DIFFSCOPE_COREPLUGIN_FREEPARAMETERSELECTIONMODEL_H
#define DIFFSCOPE_COREPLUGIN_FREEPARAMETERSELECTIONMODEL_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace dspx {
    class SelectionModel;
    class SingingClip;
}

namespace Core {
    class FreeParameterSelectionModelPrivate;

    class CORE_EXPORT FreeParameterSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(FreeParameterSelectionModel)

        Q_PROPERTY(dspx::SingingClip *singingClip READ singingClip NOTIFY contextChanged)
        Q_PROPERTY(QString parameterId READ parameterId NOTIFY contextChanged)
        Q_PROPERTY(QString displayName READ displayName NOTIFY contextChanged)
        Q_PROPERTY(Layer layer READ layer NOTIFY contextChanged)
        Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
        Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)
        Q_PROPERTY(int start READ start NOTIFY rangeChanged)
        Q_PROPERTY(int end READ end NOTIFY rangeChanged)

    public:
        enum Layer {
            EditedLayer,
            TransformLayer,
        };
        Q_ENUM(Layer)

        ~FreeParameterSelectionModel() override;

        dspx::SingingClip *singingClip() const;
        QString parameterId() const;
        QString displayName() const;
        Layer layer() const;
        bool isActive() const;
        bool hasSelection() const;
        int start() const;
        int end() const;

        Q_INVOKABLE void setContext(dspx::SingingClip *singingClip, const QString &parameterId,
                                    const QString &displayName = {}, Layer layer = EditedLayer);
        Q_INVOKABLE void clearContext();
        Q_INVOKABLE void setRange(int start, int end);
        Q_INVOKABLE void clear();

    Q_SIGNALS:
        void contextChanged();
        void activeChanged();
        void hasSelectionChanged();
        void rangeChanged();

    private:
        friend class DspxDocument;
        explicit FreeParameterSelectionModel(dspx::SelectionModel *selectionModel, QObject *parent = nullptr);
        QScopedPointer<FreeParameterSelectionModelPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_FREEPARAMETERSELECTIONMODEL_H
