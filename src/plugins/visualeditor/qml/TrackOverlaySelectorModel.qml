import QtQml
import QtQml.Models

import DiffScope.DspxModel as DspxModel

ListModel {
    id: model
    required property DspxModel.TrackList trackList

    readonly property Component delegate: QtObject {
        required property DspxModel.Track track
        property bool overlayVisible: true
    }

    readonly property Connections connections: Connections {
        target: model.trackList
        
        function onItemInserted(index, track) {
            const item = model.delegate.createObject(model, { track: track });
            model.insert(index, {data: item});
        }
        
        function onItemRemoved(index, track) {
            let o = model.get(index);
            o.data.destroy()
            model.remove(index, 1);
        }
        
        function onRotated(leftIndex, middleIndex, rightIndex) {
            const from = middleIndex;
            const to = leftIndex;
            const n = rightIndex - middleIndex;
            model.move(from, to, n);
        }
    }

    Component.onCompleted: {
        const tracks = model.trackList.items;
        for (let i = 0; i < tracks.length; ++i) {
            const item = model.delegate.createObject(model, { track: tracks[i] });
            model.append({data: item});
        }
    }

}