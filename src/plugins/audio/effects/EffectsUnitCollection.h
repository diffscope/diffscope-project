// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EFFECTSUNITCOLLECTION_H
#define DIFFSCOPE_AUDIO_EFFECTSUNITCOLLECTION_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <qqmlintegration.h>

#include <audio/audioglobal.h>

class QJSEngine;
class QQmlEngine;

namespace Audio {
    namespace Internal {
        class AudioPlugin;
    }

    class EffectsUnitClass;
    class EffectsUnitCollectionPrivate;

    class AUDIO_EXPORT EffectsUnitCollection : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(EffectsUnitCollection)
        Q_PROPERTY(QStringList effectsUnitIds READ effectsUnitIds NOTIFY effectsUnitIdsChanged)

    public:
        ~EffectsUnitCollection() override;

        static EffectsUnitCollection *instance();
        static EffectsUnitCollection *create(QQmlEngine *, QJSEngine *);

        bool registerEffectsUnitClass(const QString &id, EffectsUnitClass *effectsUnitClass);
        Q_INVOKABLE EffectsUnitClass *effectsUnitClass(const QString &id) const;
        QStringList effectsUnitIds() const;

    Q_SIGNALS:
        void effectsUnitClassRegistered(const QString &id, EffectsUnitClass *effectsUnitClass);
        void effectsUnitIdsChanged();

    private:
        friend class Internal::AudioPlugin;
        explicit EffectsUnitCollection(QObject *parent = nullptr);

        QScopedPointer<EffectsUnitCollectionPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_EFFECTSUNITCOLLECTION_H
