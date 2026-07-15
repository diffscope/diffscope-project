#ifndef DIFFSCOPE_COREPLUGIN_VIRTUALSINGERPROPERTYEDITORHELPER_H
#define DIFFSCOPE_COREPLUGIN_VIRTUALSINGERPROPERTYEDITORHELPER_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace Core {

    class ProjectWindowInterface;

    namespace Internal {

        class CORE_EXPORT VirtualSingerPropertyEditorHelper : public QObject {
            Q_OBJECT
            QML_ELEMENT

        public:
            explicit VirtualSingerPropertyEditorHelper(QObject *parent = nullptr);
            ~VirtualSingerPropertyEditorHelper() override;

            Q_INVOKABLE void editVirtualSinger(ProjectWindowInterface *windowHandle);

        private Q_SLOTS:
            void handleDialogAccepted();

        private:
            bool m_dialogAccepted{};
        };

    }

}

#endif // DIFFSCOPE_COREPLUGIN_VIRTUALSINGERPROPERTYEDITORHELPER_H
