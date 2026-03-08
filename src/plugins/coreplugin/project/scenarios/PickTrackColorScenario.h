#ifndef DIFFSCOPE_COREPLUGIN_PICKTRACKCOLORSCENARIO_H
#define DIFFSCOPE_COREPLUGIN_PICKTRACKCOLORSCENARIO_H

#include <coreplugin/DocumentEditScenario.h>

namespace dspx {
    class Track;
}

namespace Core {

    class PickTrackColorScenarioPrivate;

    class CORE_EXPORT PickTrackColorScenario : public DocumentEditScenario {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(PickTrackColorScenario)

    public:
        explicit PickTrackColorScenario(QObject *parent = nullptr);
        ~PickTrackColorScenario() override;

        Q_INVOKABLE void pickTrackColor(dspx::Track *track) const;

    private:
        QScopedPointer<PickTrackColorScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PICKTRACKCOLORSCENARIO_H