// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Clipboard.h"
#include "Clipboard_p.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

namespace DspxProject {

    void ClipboardPrivate::handleClipboardChanged() {
        Q_Q(Clipboard);
        if (mode == Clipboard::Global)
            Q_EMIT q->changed();
    }

    static QList<ClipboardData::Type> supportedTypes() {
        static const QList<ClipboardData::Type> types = {
            ClipboardData::Tempo,
            ClipboardData::Label,
            ClipboardData::KeySignature,
            ClipboardData::Track,
            ClipboardData::Clip,
            ClipboardData::Note,
            ClipboardData::DynamicMixingAnchor,
            ClipboardData::AnchorNode,
            ClipboardData::FreeParameter,
        };
        return types;
    }

    static Clipboard *m_instance = nullptr;

    Clipboard::Clipboard(QObject *parent) : QObject(parent), d_ptr(new ClipboardPrivate) {
        Q_D(Clipboard);
        Q_ASSERT(!m_instance);
        m_instance = this;
        d->q_ptr = this;
        auto *clipboard = QGuiApplication::clipboard();
        connect(clipboard, &QClipboard::dataChanged, this, [this] {
            Q_D(Clipboard);
            d->handleClipboardChanged();
        });
    }

    Clipboard::~Clipboard() {
        m_instance = nullptr;
    }

    Clipboard *Clipboard::instance() {
        return m_instance;
    }

    Clipboard::Mode Clipboard::mode() const {
        Q_D(const Clipboard);
        return d->mode;
    }

    void Clipboard::setMode(Mode mode) {
        Q_D(Clipboard);
        if (d->mode == mode)
            return;
        d->mode = mode;
        Q_EMIT changed();
    }

    void Clipboard::copy(const QList<ClipboardData> &data, QMimeData *additionalMimeData) {
        Q_D(Clipboard);
        if (d->mode == Internal) {
            d->internalData.clear();
            for (const auto &item : data) {
                auto type = item.type();
                if (d->internalData.contains(type))
                    continue;
                d->internalData.insert(type, item);
            }
            Q_EMIT changed();
            return;
        }

        auto *mimeData = additionalMimeData ? additionalMimeData : new QMimeData;

        for (const auto &item : data) {
            const auto type = item.type();
            const auto mimeType = ClipboardData::mimeType(type);
            if (mimeData->hasFormat(mimeType))
                continue;
            mimeData->setData(mimeType, item.toData());
        }

        QGuiApplication::clipboard()->setMimeData(mimeData);
    }

    ClipboardData Clipboard::paste(ClipboardData::Type expectedType, bool *ok) {
        if (ok)
            *ok = false;

        Q_D(Clipboard);

        if (d->mode == Internal) {
            const auto it = d->internalData.constFind(expectedType);
            if (it != d->internalData.cend()) {
                if (ok)
                    *ok = true;
                return it.value();
            }
            return {};
        }

        const auto mimeType = ClipboardData::mimeType(expectedType);
        const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
        if (!mimeData || mimeType.isEmpty() || !mimeData->hasFormat(mimeType))
            return {};

        return ClipboardData::fromData(mimeData->data(mimeType), expectedType, ok);
    }

    QList<ClipboardData::Type> Clipboard::availablePasteTypes() const {
        QList<ClipboardData::Type> types;
        Q_D(const Clipboard);

        if (d->mode == Internal) {
            return d->internalData.keys();
        }

        const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
        if (!mimeData)
            return types;

        for (const auto type : supportedTypes()) {
            const auto mimeType = ClipboardData::mimeType(type);
            if (mimeData->hasFormat(mimeType))
                types.append(type);
        }
        return types;
    }

}
