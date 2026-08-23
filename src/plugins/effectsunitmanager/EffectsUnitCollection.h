// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCOLLECTION_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCOLLECTION_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <qqmlintegration.h>

#include <effectsunitmanager/effectsunitmanagerglobal.h>

class QJSEngine;
class QQmlEngine;

namespace EffectsUnitManager {

    namespace Internal {
        class EffectsUnitManagerPlugin;
    }

    class EffectsUnitClass;
    class EffectsUnitCollectionPrivate;

    class EFFECTS_UNIT_MANAGER_EXPORT EffectsUnitCollection : public QObject {
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
        friend class Internal::EffectsUnitManagerPlugin;
        explicit EffectsUnitCollection(QObject *parent = nullptr);

        QScopedPointer<EffectsUnitCollectionPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCOLLECTION_H
