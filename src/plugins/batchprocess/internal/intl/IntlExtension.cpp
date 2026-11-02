// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "IntlExtension.h"

#include <QLoggingCategory>

#include <QJSEnginePolyfillIntl/QJSEnginePolyfillIntl.h>

#include <batchprocess/BatchProcessInterface.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcIntlExtension, "diffscope.batchprocess.intl.extension")

    IntlExtension::IntlExtension(BatchProcessInterface *batchProcessInterface, QObject *parent)
        : QObject(parent) {
        if (!batchProcessInterface->registerEngineExtension(this, [](QJSEngine *engine, ScriptExecutionContext *) {
                QJSEnginePolyfills::Intl::install(engine);
            })) {
            qFatal("Failed to register the Batch Process Intl engine extension");
        }
        qCDebug(lcIntlExtension) << "Registered Intl engine extension";
    }

}

#include "moc_IntlExtension.cpp"
