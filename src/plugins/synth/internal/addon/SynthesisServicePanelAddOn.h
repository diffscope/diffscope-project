#ifndef DIFFSCOPE_SYNTH_SYNTHESISSERVICEPANELADDON_H
#define DIFFSCOPE_SYNTH_SYNTHESISSERVICEPANELADDON_H

#include <CoreApi/windowinterface.h>

class QAbstractItemModel;

namespace Synth::Internal {

    class ServiceStatusModel;
    class SynthService;

    class SynthesisServicePanelAddOn final : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *serviceModel READ serviceModel CONSTANT)
        Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshingChanged)
    public:
        explicit SynthesisServicePanelAddOn(QObject *parent = nullptr);
        ~SynthesisServicePanelAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QAbstractItemModel *serviceModel() const;
        bool refreshing() const;

        Q_INVOKABLE void refreshAll();

    Q_SIGNALS:
        void refreshingChanged();

    private:
        SynthService *m_service{};
        ServiceStatusModel *m_model{};
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISSERVICEPANELADDON_H
