// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_FREEPARAMETERRESAMPLING_P_H
#define DIFFSCOPE_DSPXPROJECT_FREEPARAMETERRESAMPLING_P_H

#include <QList>
#include <QVariant>

namespace dspx {
    class FreeValueDataArray;
}

namespace DspxProject {

    struct FreeParameterClipboardData;

    class FreeParameterResampling {
    public:
        struct Result {
            int firstIndex{};
            QList<QVariant> values;
        };

        // Returns source samples and boundary neighbors on the source grid.
        static FreeParameterClipboardData capture(const dspx::FreeValueDataArray *array, int start, int end);
        // Returns whether the destination sampling grid requires interpolation of nonempty data.
        static bool requiresResampling(const FreeParameterClipboardData &data, int targetStart);
        // Returns samples in the translated interval using linear interpolation and preserving gaps.
        static Result resample(const FreeParameterClipboardData &data, int targetStart);
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_FREEPARAMETERRESAMPLING_P_H
