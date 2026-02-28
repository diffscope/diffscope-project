#ifndef DIFFSCOPE_COREPLUGIN_EDITKEYSIGNATURESCENARIO_H
#define DIFFSCOPE_COREPLUGIN_EDITKEYSIGNATURESCENARIO_H

#include <coreplugin/DocumentEditScenario.h>

namespace Core {

    class ProjectTimeline;

    class EditKeySignatureScenarioPrivate;

    class CORE_EXPORT EditKeySignatureScenario : public DocumentEditScenario {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditKeySignatureScenario)

        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline WRITE setProjectTimeline NOTIFY projectTimelineChanged)

    public:
        explicit EditKeySignatureScenario(QObject *parent = nullptr);
        ~EditKeySignatureScenario() override;

        ProjectTimeline *projectTimeline() const;
        void setProjectTimeline(ProjectTimeline *projectTimeline);

        Q_INVOKABLE void editKeySignature() const;
        Q_INVOKABLE void editKeySignature(int position, bool doInsertNew, int tonality, int mode, int accidentalType) const;
        Q_INVOKABLE void insertKeySignatureAt(int position) const;
        Q_INVOKABLE void modifyExistingKeySignatureAt(int position) const;

    Q_SIGNALS:
        void projectTimelineChanged();

    private:
        QScopedPointer<EditKeySignatureScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITKEYSIGNATURESCENARIO_H
