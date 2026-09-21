// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "StorageTypes.h"

namespace BatchProcess {

    bool StorageError::hasError() const {
        return code != StorageErrorCode::None;
    }

    void StorageError::clear() {
        code = StorageErrorCode::None;
        message.clear();
        operation.clear();
        storageId.clear();
        key.reset();
    }

}
