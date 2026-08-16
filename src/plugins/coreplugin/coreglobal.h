// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREGLOBAL_H
#define DIFFSCOPE_COREPLUGIN_COREGLOBAL_H

#include <QtCore/QtGlobal>

#if defined(CORE_LIBRARY)
#    define CORE_EXPORT Q_DECL_EXPORT
#else
#    define CORE_EXPORT Q_DECL_IMPORT
#endif

#endif // DIFFSCOPE_COREPLUGIN_COREGLOBAL_H
