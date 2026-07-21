import QtQml
import QtQuick

import DiffScope.Core

SegmentedRatioSlider {
    id: control

    required property SourcesPickerModel sourcesModel
    required property var mixedSingerIndex

    readonly property bool mixedSingerIndexValid: {
        sourcesModel.revision
        return Boolean(mixedSingerIndex && sourcesModel.indexAlive(mixedSingerIndex))
    }

    function singerNames() {
        const result = []
        if (!mixedSingerIndexValid)
            return result
        const count = sourcesModel.rowCount(mixedSingerIndex)
        for (let row = 0; row < count; ++row)
            result.push(sourcesModel.displayName(sourcesModel.index(row, 0, mixedSingerIndex)))
        return result
    }

    names: {
        const currentRevision = sourcesModel.revision
        return currentRevision >= 0 ? singerNames() : []
    }
    values: {
        const currentRevision = sourcesModel.revision
        return mixedSingerIndexValid && currentRevision >= 0
               ? sourcesModel.ratios(mixedSingerIndex) : []
    }

    onAdjacentRatioModified: (leftIndex, leftRatio) =>
        sourcesModel.setAdjacentRatios(mixedSingerIndex, leftIndex, leftRatio)
}
