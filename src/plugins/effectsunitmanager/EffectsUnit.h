// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNIT_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNIT_H

#include <memory>

#include <QJsonValue>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <effectsunitmanager/effectsunitmanagerglobal.h>

class QQuickItem;

namespace talcs {
    class AudioSource;
}

namespace EffectsUnitManager {

    class EffectsUnitPrivate;

    class EFFECTS_UNIT_MANAGER_EXPORT EffectsUnit : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(EffectsUnit)
        Q_PROPERTY(QQuickItem *editor READ editor CONSTANT)

    public:
        ~EffectsUnit() override;

        QQuickItem *editor() const;
        talcs::AudioSource *processor() const;

        virtual QJsonValue getState() const = 0;
        virtual void setState(const QJsonValue &state) = 0;

    Q_SIGNALS:
        void updated();

    protected:
        explicit EffectsUnit(QObject *parent = nullptr);

        void setEditor(QQuickItem *editor);
        void setProcessor(std::unique_ptr<talcs::AudioSource> processor);

    private:
        QScopedPointer<EffectsUnitPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNIT_H
