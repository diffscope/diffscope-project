// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_FREEPARAMETERRESAMPLING_H
#define DIFFSCOPE_COREPLUGIN_FREEPARAMETERRESAMPLING_H

#include <QList>
#include <QVariant>

#include <coreplugin/DspxClipboardData.h>

namespace dspx {
    class FreeValueDataArray;
}

namespace Core::Internal {

    struct ResampledFreeParameter {
        int firstIndex{};
        QList<QVariant> values;
    };

    // Returns source samples and boundary neighbors without resampling.
    FreeParameterClipboardData captureFreeParameter(const dspx::FreeValueDataArray *array, int start, int end);
    // Returns whether the destination sampling grid requires interpolation of nonempty data.
    bool freeParameterRequiresResampling(const FreeParameterClipboardData &data, int targetStart);
    // Returns samples in the translated interval using linear interpolation and preserving gaps.
    ResampledFreeParameter resampleFreeParameter(const FreeParameterClipboardData &data, int targetStart);

}

#endif // DIFFSCOPE_COREPLUGIN_FREEPARAMETERRESAMPLING_H
