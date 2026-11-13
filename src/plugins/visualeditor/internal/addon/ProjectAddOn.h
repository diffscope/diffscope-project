// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUAL_EDITOR_PROJECTADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_PROJECTADDON_H

#include <QList>
#include <QMetaObject>

#include <CoreApi/windowinterface.h>

namespace DspxProject {
    struct DocumentProblem;
}

namespace VisualEditor::Internal {

    class ProjectAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit ProjectAddOn(QObject *parent = nullptr);
        ~ProjectAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        bool navigateToProblem(const DspxProject::DocumentProblem &problem);
        void clearPendingNavigation();

        QList<QMetaObject::Connection> m_pendingNavigation;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_PROJECTADDON_H
