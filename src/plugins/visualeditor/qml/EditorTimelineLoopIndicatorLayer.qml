import dev.sjimo.ScopicFlow.Views

TimelineLoopIndicatorLayer {
    id: loopIndicatorLayer

    readonly property var panelInterface: ViewLayer.arrangementPanelInterface ?? ViewLayer.pianoRollPanelInterface
    readonly property ProjectViewModelContext projectViewModelContext: ViewLayer.windowHandle?.ProjectViewModelContext.context ?? null

    timeViewModel: loopIndicatorLayer.panelInterface?.timeViewModel ?? null
    timeLayoutViewModel: loopIndicatorLayer.panelInterface?.timeLayoutViewModel ?? null
    playbackViewModel: loopIndicatorLayer.projectViewModelContext?.playbackViewModel ?? null
    scrollBehaviorViewModel: loopIndicatorLayer.panelInterface?.scrollBehaviorViewModel ?? null
    timelineInteractionController: loopIndicatorLayer.panelInterface?.timelineInteractionController ?? null
}
