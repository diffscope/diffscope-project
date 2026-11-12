// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_H

#include <qqmlintegration.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <coreplugin/coreglobal.h>

namespace DspxProject {
    class ClipboardEditScenario;
    class Document;
    class EditItemsScenario;
    class EditKeySignatureScenario;
    class EditLoopScenario;
    class EditSourcesScenario;
    class EditTempoTimeSignatureScenario;
    class FillLyricsScenario;
    class InsertItemScenario;
    class PickTrackColorScenario;
    class ProjectDocumentContext;
    class ProjectTimeline;
}

namespace QAK {
    class QuickActionContext;
}

namespace Notifications {
    class NotificationMessage;
    class NotificationModel;
}

class QAbstractItemModel;
class QJSValue;

namespace Core {

    class ProjectWindowInterfacePrivate;
    class WorkspacePanelRegistry;

    class CORE_EXPORT ProjectWindowInterface : public ActionWindowInterfaceBase {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(DspxProject::ProjectTimeline *projectTimeline READ projectTimeline CONSTANT)
        Q_PROPERTY(DspxProject::ProjectDocumentContext *projectDocumentContext READ projectDocumentContext CONSTANT)
        Q_PROPERTY(DspxProject::ClipboardEditScenario *clipboardEditScenario READ clipboardEditScenario CONSTANT)
        Q_PROPERTY(DspxProject::EditItemsScenario *editItemsScenario READ editItemsScenario CONSTANT)
        Q_PROPERTY(DspxProject::EditKeySignatureScenario *editKeySignatureScenario READ editKeySignatureScenario CONSTANT)
        Q_PROPERTY(DspxProject::EditLoopScenario *editLoopScenario READ editLoopScenario CONSTANT)
        Q_PROPERTY(DspxProject::EditSourcesScenario *editSourcesScenario READ editSourcesScenario CONSTANT)
        Q_PROPERTY(DspxProject::EditTempoTimeSignatureScenario *editTempoTimeSignatureScenario READ editTempoTimeSignatureScenario CONSTANT)
        Q_PROPERTY(DspxProject::FillLyricsScenario *fillLyricsScenario READ fillLyricsScenario CONSTANT)
        Q_PROPERTY(DspxProject::InsertItemScenario *insertItemScenario READ insertItemScenario CONSTANT)
        Q_PROPERTY(DspxProject::PickTrackColorScenario *pickTrackColorScenario READ pickTrackColorScenario CONSTANT)
        Q_PROPERTY(Notifications::NotificationModel *notifications READ notifications CONSTANT)
        Q_DECLARE_PRIVATE(ProjectWindowInterface)
    public:
        static ProjectWindowInterface *of(const DspxProject::Document *document);

        DspxProject::ProjectTimeline *projectTimeline() const;

        DspxProject::ProjectDocumentContext *projectDocumentContext() const;
        WorkspacePanelRegistry *workspacePanelRegistry() const;

        DspxProject::ClipboardEditScenario *clipboardEditScenario() const;
        DspxProject::EditItemsScenario *editItemsScenario() const;
        DspxProject::EditKeySignatureScenario *editKeySignatureScenario() const;
        DspxProject::EditLoopScenario *editLoopScenario() const;
        DspxProject::EditSourcesScenario *editSourcesScenario() const;
        DspxProject::EditTempoTimeSignatureScenario *editTempoTimeSignatureScenario() const;
        DspxProject::FillLyricsScenario *fillLyricsScenario() const;
        DspxProject::InsertItemScenario *insertItemScenario() const;
        DspxProject::PickTrackColorScenario *pickTrackColorScenario() const;

        // Returns global notifications and notifications for this window.
        Notifications::NotificationModel *notifications() const;

        enum NotificationBubbleMode {
            NormalBubble,
            DoNotShowBubble,
            AutoHide,
        };
        Q_ENUM(NotificationBubbleMode)
        Q_INVOKABLE void sendNotification(Notifications::NotificationMessage *message, NotificationBubbleMode mode = NormalBubble);
        Q_INVOKABLE void sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title, const QString &text, NotificationBubbleMode mode = NormalBubble);

        Q_INVOKABLE void boundTimelineRangeHint();

        Q_INVOKABLE void sendOperationConflictNotification();

    protected:
        QWindow *createWindow(QObject *parent) const override;

        explicit ProjectWindowInterface(DspxProject::ProjectDocumentContext *projectDocumentContext, QObject *parent = nullptr);
        explicit ProjectWindowInterface(ProjectWindowInterfacePrivate &d, QObject *parent = nullptr);
        ~ProjectWindowInterface() override;

        friend class ExecutiveInterfaceRegistry<ProjectWindowInterface>;

    private:
        QScopedPointer<ProjectWindowInterfacePrivate> d_ptr;
    };

    class CORE_EXPORT ProjectWindowInterfaceRegistry : public ExecutiveInterfaceRegistry<ProjectWindowInterface> {
    public:
        static ProjectWindowInterfaceRegistry *instance();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_H
