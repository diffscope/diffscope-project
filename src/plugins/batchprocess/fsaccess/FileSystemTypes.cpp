// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "FileSystemTypes.h"

namespace BatchProcess {

    bool FileSystemError::hasError() const {
        return kind != FileSystemErrorKind::None;
    }

    void FileSystemError::clear() {
        *this = {};
    }

}
