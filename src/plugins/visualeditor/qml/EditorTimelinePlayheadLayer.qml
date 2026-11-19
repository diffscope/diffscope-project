import dev.sjimo.ScopicFlow.Views

TimelinePlayheadLayer {
    id: playheadLayer

    readonly property var panelInterface: ViewLayer.arrangementPanelInterface ?? ViewLayer.pianoRollPanelInterface
    readonly property ProjectViewModelContext projectViewModelContext: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null

    timeViewModel: playheadLayer.panelInterface?.timeViewModel ?? null
    timeLayoutViewModel: playheadLayer.panelInterface?.timeLayoutViewModel ?? null
    playbackViewModel: playheadLayer.projectViewModelContext?.playbackViewModel ?? null
}
