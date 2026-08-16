// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "HashHelper.h"

#include <xxhash.h>

#include <QFile>

namespace Audio::Internal {

    namespace {
        struct XXH3StateDeleter {
            void operator()(XXH3_state_t* state) const noexcept {
                XXH3_freeState(state);
            }
        };
        using XXH3StatePtr = std::unique_ptr<XXH3_state_t, XXH3StateDeleter>;

    }

    QString HashHelper::digest(const QString &filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        XXH3StatePtr state{XXH3_createState()};
        if (XXH3_128bits_reset(state.get()) != XXH_OK) {
            return {};
        }
        constexpr std::size_t BufferSize = 256 * 1024;
        std::array<char, BufferSize> buffer;
        while (true) {
            const qint64 size = file.read(buffer.data(),static_cast<qint64>(buffer.size()));
            if (size < 0) {
                return {};
            }
            if (size == 0) {
                break;
            }
            if (XXH3_128bits_update(state.get(), buffer.data(), static_cast<std::size_t>(size)) != XXH_OK) {
                return {};
            }
        }
        const auto result = XXH3_128bits_digest(state.get());
        return QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(&result), sizeof(result)).toBase64(QByteArray::Base64UrlEncoding));
    }

}
