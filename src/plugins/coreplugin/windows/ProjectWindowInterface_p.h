// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_P_H

#include <coreplugin/ProjectWindowInterface.h>

namespace Core {

    class ProjectWindowInterfacePrivate {
        Q_DECLARE_PUBLIC(ProjectWindowInterface)
    public:
        ProjectWindowInterface *q_ptr;
        Notifications::NotificationModel *notifications{};
        DspxProject::ProjectTimeline *projectTimeline;
        DspxProject::ProjectDocumentContext *projectDocumentContext;
        WorkspacePanelRegistry *workspacePanelRegistry = nullptr;
        DspxProject::ClipboardEditScenario *clipboardEditScenario = nullptr;
        DspxProject::EditItemsScenario *editItemsScenario = nullptr;
        DspxProject::EditKeySignatureScenario *editKeySignatureScenario = nullptr;
        DspxProject::EditLoopScenario *editLoopScenario = nullptr;
        DspxProject::EditSourcesScenario *editSourcesScenario = nullptr;
        DspxProject::EditTempoTimeSignatureScenario *editTempoTimeSignatureScenario = nullptr;
        DspxProject::FillLyricsScenario *fillLyricsScenario = nullptr;
        DspxProject::InsertItemScenario *insertItemScenario = nullptr;
        DspxProject::PickTrackColorScenario *pickTrackColorScenario = nullptr;
        Notifications::NotificationMessage *operationConflictMessage{};

        void init();
        void initEditScenarios();
        void initActionContext();
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_P_H
