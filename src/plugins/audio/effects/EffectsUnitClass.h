// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EFFECTSUNITCLASS_H
#define DIFFSCOPE_AUDIO_EFFECTSUNITCLASS_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <audio/audioglobal.h>

namespace Audio {

    class EffectsUnit;
    class EffectsUnitClassPrivate;

    class AUDIO_EXPORT EffectsUnitClass : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(EffectsUnitClass)
        Q_PROPERTY(QString name READ name CONSTANT)

    public:
        ~EffectsUnitClass() override;

        QString name() const;
        virtual EffectsUnit *create(QObject *parent = nullptr) const = 0;

    protected:
        explicit EffectsUnitClass(const QString &name, QObject *parent = nullptr);

    private:
        QScopedPointer<EffectsUnitClassPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_EFFECTSUNITCLASS_H
