#ifndef DIFFSCOPE_COREPLUGIN_DSPXVISUALIZATIONMODEL_P_H
#define DIFFSCOPE_COREPLUGIN_DSPXVISUALIZATIONMODEL_P_H

#include <QStandardItemModel>

#include <coreplugin/coreglobal.h>

namespace QDspx {
    struct Model;
    struct Content;
    struct Global;
    struct Master;
    struct BusControl;
    struct Timeline;
    struct Label;
    struct Tempo;
    struct TimeSignature;
    struct Track;
    struct TrackControl;
    using Workspace = QMap<QString, QJsonObject>;
    using Sources = QMap<QString, QJsonObject>;
    struct Clip;
    struct AudioClip;
    struct SingingClip;
    struct ClipTime;
    struct Note;
    struct Pronunciation;
    struct Phonemes;
    struct Phoneme;
    struct Vibrato;
    struct VibratoPoints;
    struct ControlPoint;
    struct Param;
    using Params = QMap<QString, Param>;
    struct ParamCurve;
    struct ParamCurveAnchor;
    struct ParamCurveFree;
    struct AnchorNode;
}

namespace Core {

    class CORE_EXPORT DspxVisualizationModel : public QStandardItemModel {
        Q_OBJECT
    public:
        explicit DspxVisualizationModel(QObject *parent = nullptr);
        ~DspxVisualizationModel() override;

        void generate(const QDspx::Model &model);

    private:
        template <typename T>
        QStandardItem *createItem(const T &entity) const {
            static_assert(sizeof(T) == 0);
            return {};
        }

    };

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Content &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Global &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Master &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::BusControl &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Timeline &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Label &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Tempo &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::TimeSignature &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Track &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::TrackControl &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Workspace &entity) const;

    // template <>
    // QStandardItem *DspxVisualizationModel::createItem(const QDspx::Sources &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Clip &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::AudioClip &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::SingingClip &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::ClipTime &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Note &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Pronunciation &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Phonemes &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Phoneme &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Vibrato &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::VibratoPoints &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::ControlPoint &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::Param &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::ParamCurve &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::ParamCurveAnchor &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::ParamCurveFree &entity) const;

    template <>
    QStandardItem *DspxVisualizationModel::createItem(const QDspx::AnchorNode &entity) const;

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXVISUALIZATIONMODEL_P_H
