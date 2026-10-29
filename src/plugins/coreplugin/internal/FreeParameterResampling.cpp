// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "FreeParameterResampling.h"

#include <dspxmodelORM/FreeValueDataArray.h>

#include <coreplugin/ArchitectureInfo.h>

namespace Core::Internal {

    namespace {

        int ceilToIndex(int position) {
            const int step = dspx::FreeValueDataArray::step();
            return position / step + (position % step > 0);
        }

        QVariant sampleFreeParameter(const FreeParameterClipboardData &data, int position) {
            const int step = dspx::FreeValueDataArray::step();
            const int relativePosition = position - data.offset;
            if (relativePosition < 0)
                return {};
            const int leftIndex = relativePosition / step;
            if (leftIndex >= data.values.size() || !data.values.at(leftIndex).isValid())
                return {};
            const auto &leftValue = data.values.at(leftIndex);
            if (relativePosition % step == 0)
                return leftValue;
            const int rightIndex = leftIndex + 1;
            if (rightIndex >= data.values.size() || !data.values.at(rightIndex).isValid())
                return {};
            const double fraction = static_cast<double>(relativePosition % step) / step;
            const double left = ParameterInfo::fromDspxModelValue(leftValue.toInt());
            const double right = ParameterInfo::fromDspxModelValue(data.values.at(rightIndex).toInt());
            return ParameterInfo::toDspxModelValue(left + (right - left) * fraction);
        }

    }

    FreeParameterClipboardData captureFreeParameter(const dspx::FreeValueDataArray *array, int start, int end) {
        const int step = dspx::FreeValueDataArray::step();
        const int firstIndex = start / step;
        const int endIndex = ceilToIndex(end) + 1;
        return {
            .length = end - start,
            .offset = firstIndex * step - start,
            .values = array->slice(firstIndex, endIndex - firstIndex),
        };
    }

    bool freeParameterRequiresResampling(const FreeParameterClipboardData &data, int targetStart) {
        const int step = dspx::FreeValueDataArray::step();
        if ((targetStart + data.offset) % step == 0)
            return false;
        for (int index = 0; index < data.values.size(); ++index) {
            if (!data.values.at(index).isValid())
                continue;
            const qint64 position = static_cast<qint64>(data.offset) + static_cast<qint64>(index) * step;
            if (position >= 0 && position < data.length)
                return true;
            if (index + 1 < data.values.size() && data.values.at(index + 1).isValid() &&
                position < data.length && position + step > 0) {
                return true;
            }
        }
        return false;
    }

    ResampledFreeParameter resampleFreeParameter(const FreeParameterClipboardData &data, int targetStart) {
        const int step = dspx::FreeValueDataArray::step();
        ResampledFreeParameter result;
        result.firstIndex = ceilToIndex(targetStart);
        const int endIndex = ceilToIndex(targetStart + data.length);
        result.values.reserve(endIndex - result.firstIndex);
        for (int index = result.firstIndex; index < endIndex; ++index)
            result.values.append(sampleFreeParameter(data, index * step - targetStart));
        return result;
    }

}
