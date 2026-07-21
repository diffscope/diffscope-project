#ifndef DIFFSCOPE_COREPLUGIN_DYNAMICMIXINGANCHORPROPERTYMAPPER_H
#define DIFFSCOPE_COREPLUGIN_DYNAMICMIXINGANCHORPROPERTYMAPPER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {

    class DynamicMixingAnchorPropertyMapperPrivate;

    class CORE_EXPORT DynamicMixingAnchorPropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(DynamicMixingAnchorPropertyMapper)
        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant position READ position NOTIFY positionChanged)
        Q_PROPERTY(int voiceCount READ voiceCount NOTIFY singersChanged)
        Q_PROPERTY(QObjectList singers READ singers NOTIFY singersChanged)
        Q_PROPERTY(QVariant ratios READ ratios NOTIFY ratiosChanged)

    public:
        explicit DynamicMixingAnchorPropertyMapper(QObject *parent = nullptr);
        ~DynamicMixingAnchorPropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant position() const;
        int voiceCount() const;
        QObjectList singers() const;
        QVariant ratios() const;

        Q_INVOKABLE QVariant ratioAt(int singerIndex) const;
        Q_INVOKABLE QVariant maximumRatioAt(int singerIndex) const;
        Q_INVOKABLE bool setSingerRatio(int singerIndex, double ratio);
        Q_INVOKABLE bool setAdjacentRatios(int leftSingerIndex, double leftRatio);

    Q_SIGNALS:
        void selectionModelChanged();
        void positionChanged();
        void singersChanged();
        void ratiosChanged();

    private:
        QScopedPointer<DynamicMixingAnchorPropertyMapperPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_DYNAMICMIXINGANCHORPROPERTYMAPPER_H
