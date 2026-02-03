#ifndef DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_H
#define DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_H

#include <coreplugin/DocumentEditScenario.h>

namespace Core {

    class EditMetadataScenarioPrivate;

    class CORE_EXPORT EditMetadataScenario : public DocumentEditScenario {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditMetadataScenario)

    public:
        explicit EditMetadataScenario(QObject *parent = nullptr);
        ~EditMetadataScenario() override;

        Q_INVOKABLE void editMetadata() const;

    private:
        QScopedPointer<EditMetadataScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_H
