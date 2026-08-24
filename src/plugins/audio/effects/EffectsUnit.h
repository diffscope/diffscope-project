// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EFFECTSUNIT_H
#define DIFFSCOPE_AUDIO_EFFECTSUNIT_H

#include <memory>

#include <QJsonValue>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <audio/audioglobal.h>

class QQuickItem;

namespace talcs {
    class AudioSource;
}

namespace Audio {

    class EffectsUnitPrivate;

    class AUDIO_EXPORT EffectsUnit : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(EffectsUnit)
        Q_PROPERTY(QQuickItem *editor READ editor CONSTANT)

    public:
        ~EffectsUnit() override;

        QQuickItem *editor() const;
        talcs::AudioSource *processor() const;

        /**
         * Clears internal processing state. This method is called on the main thread.
         */
        virtual void refresh();

        virtual QJsonValue getState() const = 0;

        /**
         * Sets the effect state. A null value resets the effect to its default state.
         */
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

#endif // DIFFSCOPE_AUDIO_EFFECTSUNIT_H
