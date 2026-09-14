// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ScriptMetadata.h"

namespace BatchProcess {

    ScriptMetadata::ScriptMetadata() = default;

    QString ScriptMetadata::id() const {
        return m_id;
    }

    QString ScriptMetadata::name() const {
        return m_name;
    }

    QString ScriptMetadata::version() const {
        return m_version;
    }

    QString ScriptMetadata::description() const {
        return m_description;
    }

    QString ScriptMetadata::author() const {
        return m_author;
    }

    bool ScriptMetadata::isValid() const {
        return !m_id.isEmpty();
    }

}
