// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsUnit.h"

#include <utility>

#include <QQuickItem>

#include <TalcsCore/AudioSource.h>

#include <effectsunitmanager/private/EffectsUnit_p.h>

namespace EffectsUnitManager {

    EffectsUnit::EffectsUnit(QObject *parent)
        : QObject(parent), d_ptr(new EffectsUnitPrivate) {
        Q_D(EffectsUnit);
        d->q_ptr = this;
    }

    EffectsUnit::~EffectsUnit() = default;

    QQuickItem *EffectsUnit::editor() const {
        Q_D(const EffectsUnit);
        return d->editor;
    }

    talcs::AudioSource *EffectsUnit::processor() const {
        Q_D(const EffectsUnit);
        return d->processor.get();
    }

    void EffectsUnit::setEditor(QQuickItem *editor) {
        Q_D(EffectsUnit);
        Q_ASSERT(editor);
        Q_ASSERT(!d->editor);
        editor->setParent(this);
        d->editor = editor;
    }

    void EffectsUnit::setProcessor(std::unique_ptr<talcs::AudioSource> processor) {
        Q_D(EffectsUnit);
        Q_ASSERT(processor);
        Q_ASSERT(!d->processor);
        d->processor = std::move(processor);
    }

}

#include "moc_EffectsUnit.cpp"
