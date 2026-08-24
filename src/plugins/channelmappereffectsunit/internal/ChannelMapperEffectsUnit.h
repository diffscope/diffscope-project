// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_CHANNEL_MAPPER_EFFECTS_UNIT_CHANNELMAPPEREFFECTSUNIT_H
#define DIFFSCOPE_CHANNEL_MAPPER_EFFECTS_UNIT_CHANNELMAPPEREFFECTSUNIT_H

#include <QJsonValue>
#include <QPointer>
#include <qqmlintegration.h>

#include <audio/EffectsUnit.h>
#include <audio/EffectsUnitClass.h>

class QQmlComponent;

namespace ChannelMapperEffectsUnit::Internal {

    class ChannelMapperProcessor;

    class ChannelMapperEffectsUnit : public Audio::EffectsUnit {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(double leftLeftMixPercent READ leftLeftMixPercent NOTIFY leftLeftMixPercentChanged)
        Q_PROPERTY(double leftRightMixPercent READ leftRightMixPercent NOTIFY leftRightMixPercentChanged)
        Q_PROPERTY(double rightLeftMixPercent READ rightLeftMixPercent NOTIFY rightLeftMixPercentChanged)
        Q_PROPERTY(double rightRightMixPercent READ rightRightMixPercent NOTIFY rightRightMixPercentChanged)

    public:
        explicit ChannelMapperEffectsUnit(QQmlComponent *editorComponent, QObject *parent = nullptr);
        ~ChannelMapperEffectsUnit() override;

        double leftLeftMixPercent() const;
        double leftRightMixPercent() const;
        double rightLeftMixPercent() const;
        double rightRightMixPercent() const;

        QJsonValue getState() const override;
        void setState(const QJsonValue &state) override;

        Q_INVOKABLE void previewLeftLeftMixPercent(double value);
        Q_INVOKABLE void previewLeftRightMixPercent(double value);
        Q_INVOKABLE void previewRightLeftMixPercent(double value);
        Q_INVOKABLE void previewRightRightMixPercent(double value);
        Q_INVOKABLE void commitPreview();
        Q_INVOKABLE void setLeftLeftMixPercent(double value);
        Q_INVOKABLE void setLeftRightMixPercent(double value);
        Q_INVOKABLE void setRightLeftMixPercent(double value);
        Q_INVOKABLE void setRightRightMixPercent(double value);

    Q_SIGNALS:
        void leftLeftMixPercentChanged();
        void leftRightMixPercentChanged();
        void rightLeftMixPercentChanged();
        void rightRightMixPercentChanged();

    private:
        bool previewMix(double &member, double value);
        void updateProcessor();

        ChannelMapperProcessor *m_processor{};
        double m_committedLeftLeftMixPercent{100.0};
        double m_committedLeftRightMixPercent{0.0};
        double m_committedRightLeftMixPercent{0.0};
        double m_committedRightRightMixPercent{100.0};
        double m_leftLeftMixPercent{100.0};
        double m_leftRightMixPercent{0.0};
        double m_rightLeftMixPercent{0.0};
        double m_rightRightMixPercent{100.0};
    };

    class ChannelMapperEffectsUnitClass : public Audio::EffectsUnitClass {
        Q_OBJECT

    public:
        explicit ChannelMapperEffectsUnitClass(QObject *parent = nullptr);
        ~ChannelMapperEffectsUnitClass() override;

        Audio::EffectsUnit *create(QObject *parent = nullptr) const override;

    private:
        QQmlComponent *m_editorComponent{};
    };

}

#endif // DIFFSCOPE_CHANNEL_MAPPER_EFFECTS_UNIT_CHANNELMAPPEREFFECTSUNIT_H
