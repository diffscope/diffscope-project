// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_CHILDPROCESS_H
#define DIFFSCOPE_COREPLUGIN_CHILDPROCESS_H

#include <coreplugin/coreglobal.h>

class QProcess;
class QString;

namespace Core {

    // Configures a stopped process to terminate on parent exit on Windows and Linux. Returns whether configuration succeeded.
    // Replaces the platform-specific process modifier and retains Windows resources until reconfiguration or destruction.
    // Linux protection applies to the direct child and the lifetime of the thread that starts it.
    // Returns true without changes on other platforms. Reports configuration failures through errorMessage if provided.
    CORE_EXPORT bool configureChildProcess(QProcess *process, QString *errorMessage = nullptr);

}

#endif // DIFFSCOPE_COREPLUGIN_CHILDPROCESS_H
