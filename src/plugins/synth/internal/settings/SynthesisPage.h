#ifndef DIFFSCOPE_SYNTH_SYNTHESISPAGE_H
#define DIFFSCOPE_SYNTH_SYNTHESISPAGE_H

#include <CoreApi/isettingpage.h>

namespace Synth::Internal {

    class SynthesisPage final : public Core::ISettingPage {
        Q_OBJECT
    public:
        explicit SynthesisPage(QObject *parent = nullptr);
        ~SynthesisPage() override;

        QString sortKeyword() const override;
        QObject *widget() override;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPAGE_H
