#ifndef DIFFSCOPE_VISUALEDITOR_PARAMETERDIVISIONITEM_H
#define DIFFSCOPE_VISUALEDITOR_PARAMETERDIVISIONITEM_H

#include <QColor>
#include <QQuickPaintedItem>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <coreplugin/ArchitectureInfo.h>

#include <visualeditor/visualeditorglobal.h>

namespace VisualEditor {

    class ParameterDivisionItemPrivate;

    class VISUAL_EDITOR_EXPORT ParameterDivisionItem : public QQuickPaintedItem {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ParameterDivisionItem)
        Q_PROPERTY(Core::ParameterInfo parameterInfo READ parameterInfo WRITE setParameterInfo NOTIFY parameterInfoChanged)
        Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
        Q_PROPERTY(qreal lineLength READ lineLength WRITE setLineLength NOTIFY lineLengthChanged)

    public:
        explicit ParameterDivisionItem(QQuickItem *parent = nullptr);
        ~ParameterDivisionItem() override;

        Core::ParameterInfo parameterInfo() const;
        void setParameterInfo(const Core::ParameterInfo &parameterInfo);
        QColor color() const;
        void setColor(const QColor &color);
        qreal lineLength() const;
        void setLineLength(qreal lineLength);

        void paint(QPainter *painter) override;

    Q_SIGNALS:
        void parameterInfoChanged();
        void colorChanged();
        void lineLengthChanged();

    private:
        QScopedPointer<ParameterDivisionItemPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_VISUALEDITOR_PARAMETERDIVISIONITEM_H
