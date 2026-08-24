// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_GAIN_EFFECTS_UNIT_GAINEFFECTSUNIT_H
#define DIFFSCOPE_GAIN_EFFECTS_UNIT_GAINEFFECTSUNIT_H

#include <QJsonValue>
#include <QPointer>
#include <qqmlintegration.h>

#include <effectsunitmanager/EffectsUnit.h>
#include <effectsunitmanager/EffectsUnitClass.h>

class QQmlComponent;

namespace GainEffectsUnit::Internal {

    class GainProcessor;

    class GainEffectsUnit : public EffectsUnitManager::EffectsUnit {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(double leftGainDb READ leftGainDb NOTIFY leftGainDbChanged)
        Q_PROPERTY(double rightGainDb READ rightGainDb NOTIFY rightGainDbChanged)
        Q_PROPERTY(bool channelsLinked READ channelsLinked NOTIFY channelsLinkedChanged)

    public:
        explicit GainEffectsUnit(QQmlComponent *editorComponent, QObject *parent = nullptr);
        ~GainEffectsUnit() override;

        double leftGainDb() const;
        double rightGainDb() const;
        bool channelsLinked() const;

        QJsonValue getState() const override;
        void setState(const QJsonValue &state) override;

        Q_INVOKABLE void previewLeftGainDb(double value);
        Q_INVOKABLE void previewRightGainDb(double value);
        Q_INVOKABLE void commitPreview();
        Q_INVOKABLE void setLeftGainDb(double value);
        Q_INVOKABLE void setRightGainDb(double value);
        Q_INVOKABLE void setChannelsLinked(bool linked);

    Q_SIGNALS:
        void leftGainDbChanged();
        void rightGainDbChanged();
        void channelsLinkedChanged();

    private:
        void previewGain(int channel, double value);
        void updateProcessor();

        GainProcessor *m_processor{};
        double m_committedLeftGainDb{};
        double m_committedRightGainDb{};
        double m_leftGainDb{};
        double m_rightGainDb{};
        bool m_channelsLinked{true};
    };

    class GainEffectsUnitClass : public EffectsUnitManager::EffectsUnitClass {
        Q_OBJECT

    public:
        explicit GainEffectsUnitClass(QObject *parent = nullptr);
        ~GainEffectsUnitClass() override;

        EffectsUnitManager::EffectsUnit *create(QObject *parent = nullptr) const override;

    private:
        QQmlComponent *m_editorComponent{};
    };

}

#endif // DIFFSCOPE_GAIN_EFFECTS_UNIT_GAINEFFECTSUNIT_H
