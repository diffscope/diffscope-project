// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHGLOBAL_H
#define DIFFSCOPE_SYNTH_SYNTHGLOBAL_H

#include <QtCore/QtGlobal>

#if defined(SYNTH_LIBRARY)
#  define SYNTH_EXPORT Q_DECL_EXPORT
#else
#  define SYNTH_EXPORT Q_DECL_IMPORT
#endif

#endif // DIFFSCOPE_SYNTH_SYNTHGLOBAL_H
