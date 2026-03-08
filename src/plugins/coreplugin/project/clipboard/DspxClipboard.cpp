#include "DspxClipboard.h"
#include "DspxClipboard_p.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

namespace Core {

    void DspxClipboardPrivate::handleClipboardChanged() {
        Q_Q(DspxClipboard);
        if (mode == DspxClipboard::Global)
            Q_EMIT q->changed();
    }

    static QList<DspxClipboardData::Type> supportedTypes() {
        static const QList<DspxClipboardData::Type> types = {
            DspxClipboardData::Tempo,
            DspxClipboardData::Label,
            DspxClipboardData::KeySignature,
            DspxClipboardData::Track,
            DspxClipboardData::Clip,
            DspxClipboardData::Note
        };
        return types;
    }

    static DspxClipboard *m_instance = nullptr;

    DspxClipboard::DspxClipboard(QObject *parent) : QObject(parent), d_ptr(new DspxClipboardPrivate) {
        Q_D(DspxClipboard);
        Q_ASSERT(!m_instance);
        m_instance = this;
        d->q_ptr = this;
        auto *clipboard = QGuiApplication::clipboard();
        connect(clipboard, &QClipboard::dataChanged, this, [this] {
            Q_D(DspxClipboard);
            d->handleClipboardChanged();
        });
    }

    DspxClipboard::~DspxClipboard() {
        m_instance = nullptr;
    }

    DspxClipboard *DspxClipboard::instance() {
        return m_instance;
    }

    DspxClipboard::Mode DspxClipboard::mode() const {
        Q_D(const DspxClipboard);
        return d->mode;
    }

    void DspxClipboard::setMode(Mode mode) {
        Q_D(DspxClipboard);
        if (d->mode == mode)
            return;
        d->mode = mode;
        Q_EMIT changed();
    }

    void DspxClipboard::copy(const QList<DspxClipboardData> &data, QMimeData *additionalMimeData) {
        Q_D(DspxClipboard);
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
            const auto mimeType = DspxClipboardData::mimeType(type);
            if (mimeType.isEmpty() || mimeData->hasFormat(mimeType))
                continue;
            mimeData->setData(mimeType, item.toData());
        }

        QGuiApplication::clipboard()->setMimeData(mimeData);
    }

    DspxClipboardData DspxClipboard::paste(DspxClipboardData::Type expectedType, bool *ok) {
        if (ok)
            *ok = false;

        Q_D(DspxClipboard);

        if (d->mode == Internal) {
            const auto it = d->internalData.constFind(expectedType);
            if (it != d->internalData.cend() && it->has_value()) {
                if (ok)
                    *ok = true;
                return it->value();
            }
            return {};
        }

        const auto mimeType = DspxClipboardData::mimeType(expectedType);
        const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
        if (!mimeData || mimeType.isEmpty() || !mimeData->hasFormat(mimeType))
            return {};

        return DspxClipboardData::fromData(mimeData->data(mimeType), expectedType, ok);
    }

    QList<DspxClipboardData::Type> DspxClipboard::availablePasteTypes() const {
        QList<DspxClipboardData::Type> types;
        Q_D(const DspxClipboard);

        if (d->mode == Internal) {
            for (auto it = d->internalData.cbegin(); it != d->internalData.cend(); ++it) {
                if (it.value().has_value())
                    types.append(it.key());
            }
            return types;
        }

        const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
        if (!mimeData)
            return types;

        for (const auto type : supportedTypes()) {
            const auto mimeType = DspxClipboardData::mimeType(type);
            if (!mimeType.isEmpty() && mimeData->hasFormat(mimeType))
                types.append(type);
        }
        return types;
    }

}
