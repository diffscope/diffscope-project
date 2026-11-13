// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROBLEMSADDON_H
#define DIFFSCOPE_COREPLUGIN_PROBLEMSADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class DocumentProblems;

    class ProblemsAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(DocumentProblems *documentProblems READ documentProblems CONSTANT)
    public:
        explicit ProblemsAddOn(QObject *parent = nullptr);
        ~ProblemsAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

        DocumentProblems *documentProblems() const;

    Q_SIGNALS:
        void showPanelRequested();

    private:
        DocumentProblems *m_documentProblems{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROBLEMSADDON_H
