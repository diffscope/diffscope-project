import dev.sjimo.ScopicFlow.Views

import DiffScope.DspxProject as DspxProject

TimelinePlayheadLayer {
    id: playheadLayer

    readonly property var panelInterface: ViewLayer.arrangementPanelInterface ?? ViewLayer.pianoRollPanelInterface
    readonly property ProjectViewModelContext projectViewModelContext: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null

    timeViewModel: playheadLayer.panelInterface?.timeViewModel ?? null
    timeLayoutViewModel: playheadLayer.panelInterface?.timeLayoutViewModel ?? null
    playbackViewModel: playheadLayer.projectViewModelContext?.playbackViewModel ?? null
    timecodeFormat: {
        switch (ViewLayer.windowHandle?.projectTimeline?.timecodeFormat) {
        case DspxProject.ProjectTimeline.AbsoluteTime:
            return TimelinePlayheadLayer.AbsoluteTime
        case DspxProject.ProjectTimeline.MusicAndAbsoluteTime:
            return TimelinePlayheadLayer.MusicAndAbsoluteTime
        default:
            return TimelinePlayheadLayer.MusicTime
        }
    }
}
