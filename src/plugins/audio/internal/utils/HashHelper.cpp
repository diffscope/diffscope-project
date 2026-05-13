#include "HashHelper.h"

#include <QCryptographicHash>
#include <QFile>

namespace Audio::Internal {

    QString HashHelper::sha512(const QString &filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }

        QCryptographicHash hash(QCryptographicHash::Sha512);
        if (!hash.addData(&file)) {
            return {};
        }
        return QString::fromLatin1(hash.result().toHex());
    }

}
