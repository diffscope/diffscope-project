#include "SynthesisPage.h"

namespace Synth::Internal {

    SynthesisPage::SynthesisPage(QObject *parent)
        : Core::ISettingPage(QStringLiteral("org.diffscope.synth.Synthesis"), parent) {
        setTitle(tr("Synthesis"));
        setDescription(tr("Configure synthesis services and parameters"));
    }

    SynthesisPage::~SynthesisPage() = default;

    QString SynthesisPage::sortKeyword() const {
        return QStringLiteral("Synthesis");
    }

    QObject *SynthesisPage::widget() {
        return nullptr;
    }

}
