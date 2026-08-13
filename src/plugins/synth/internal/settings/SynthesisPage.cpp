#include "SynthesisPage.h"

#include <QQmlComponent>
#include <QSettings>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <synth/SynthInterface.h>
#include <synth/SynthesisTaskManager.h>

namespace Synth::Internal {

    SynthesisPage::SynthesisPage(QObject *parent)
        : Core::ISettingPage(QStringLiteral("org.diffscope.synth.Synthesis"), parent) {
        setTitle(tr("Synthesis"));
        setDescription(tr("Configure synthesis behavior and storage"));
        connect(this, &SynthesisPage::valuesChanged, this, [this] {
            if (!m_loading)
                markDirty();
        });
    }

    SynthesisPage::~SynthesisPage() {
        delete m_widget;
    }

    QString SynthesisPage::sortKeyword() const {
        return QStringLiteral("Synthesis");
    }

    bool SynthesisPage::matches(const QString &word) {
        if (Core::ISettingPage::matches(word))
            return true;
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool result{};
        return matcher && QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word) && result;
    }

    QObject *SynthesisPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("SynthesisPage"));
        if (component.isError())
            qFatal() << component.errorString();
        m_widget = component.createWithInitialProperties({
            {QStringLiteral("pageHandle"), QVariant::fromValue(this)},
        });
        if (!m_widget)
            qFatal() << component.errorString();
        m_widget->setParent(this);
        return m_widget;
    }

    void SynthesisPage::beginSetting() {
        m_loading = true;
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synth"));
        m_paddingBase = settings->value(QStringLiteral("piecePaddingBaseMs"), 100.0).toDouble();
        m_paddingAdditional = settings->value(QStringLiteral("piecePaddingAdditionalMs"), 100.0).toDouble();
        m_paddingGap = settings->value(QStringLiteral("piecePaddingGapMs"), 200.0).toDouble();
        m_restLyrics = settings->value(QStringLiteral("pieceRestLyrics"), QStringList{QStringLiteral("AP"), QStringLiteral("SP")})
                           .toStringList()
                           .join(QStringLiteral(", "));
        m_parameterSampleRate = settings->value(QStringLiteral("parameterSampleRate"), 100).toInt();
        m_mixSampleRate = settings->value(QStringLiteral("mixSampleRate"), 100).toInt();
        m_cacheMaximumGiB = static_cast<int>(settings->value(QStringLiteral("cacheMaximumBytes"), qint64(10) * 1024 * 1024 * 1024).toLongLong() / (qint64(1024) * 1024 * 1024));
        m_cacheExpiryDays = settings->value(QStringLiteral("cacheExpiryDays"), 30).toInt();
        m_audioDownloadMaximumMiB = static_cast<int>(settings->value(QStringLiteral("audioDownloadMaximumBytes"), qint64(512) * 1024 * 1024).toLongLong() / (qint64(1024) * 1024));
        m_environmentTagTtlSeconds = settings->value(QStringLiteral("environmentTagTtlSeconds"), 60).toInt();
        settings->endGroup();
        Q_EMIT valuesChanged();
        m_loading = false;
        Core::ISettingPage::beginSetting();
    }

    bool SynthesisPage::accept() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synth"));
        settings->setValue(QStringLiteral("piecePaddingBaseMs"), m_paddingBase);
        settings->setValue(QStringLiteral("piecePaddingAdditionalMs"), m_paddingAdditional);
        settings->setValue(QStringLiteral("piecePaddingGapMs"), m_paddingGap);
        QStringList rest;
        for (const auto &item : m_restLyrics.split(u',', Qt::SkipEmptyParts))
            rest.append(item.trimmed());
        settings->setValue(QStringLiteral("pieceRestLyrics"), rest);
        settings->setValue(QStringLiteral("parameterSampleRate"), m_parameterSampleRate);
        settings->setValue(QStringLiteral("mixSampleRate"), m_mixSampleRate);
        settings->setValue(QStringLiteral("cacheMaximumBytes"), qint64(m_cacheMaximumGiB) * 1024 * 1024 * 1024);
        settings->setValue(QStringLiteral("cacheExpiryDays"), m_cacheExpiryDays);
        settings->setValue(QStringLiteral("audioDownloadMaximumBytes"), qint64(m_audioDownloadMaximumMiB) * 1024 * 1024);
        settings->setValue(QStringLiteral("environmentTagTtlSeconds"), m_environmentTagTtlSeconds);
        settings->endGroup();
        if (auto interface = SynthInterface::instance(); interface && interface->taskManager())
            interface->taskManager()->reloadSettings();
        return Core::ISettingPage::accept();
    }

    double SynthesisPage::paddingBase() const {
        return m_paddingBase;
    }

    void SynthesisPage::setPaddingBase(double value) {
        if (m_paddingBase != value) {
            m_paddingBase = value;
            Q_EMIT valuesChanged();
        }
    }

    double SynthesisPage::paddingAdditional() const {
        return m_paddingAdditional;
    }

    void SynthesisPage::setPaddingAdditional(double value) {
        if (m_paddingAdditional != value) {
            m_paddingAdditional = value;
            Q_EMIT valuesChanged();
        }
    }

    double SynthesisPage::paddingGap() const {
        return m_paddingGap;
    }

    void SynthesisPage::setPaddingGap(double value) {
        if (m_paddingGap != value) {
            m_paddingGap = value;
            Q_EMIT valuesChanged();
        }
    }

    QString SynthesisPage::restLyrics() const {
        return m_restLyrics;
    }

    void SynthesisPage::setRestLyrics(const QString &value) {
        if (m_restLyrics != value) {
            m_restLyrics = value;
            Q_EMIT valuesChanged();
        }
    }

    int SynthesisPage::parameterSampleRate() const {
        return m_parameterSampleRate;
    }

    void SynthesisPage::setParameterSampleRate(int value) {
        if (m_parameterSampleRate != value) {
            m_parameterSampleRate = value;
            Q_EMIT valuesChanged();
        }
    }

    int SynthesisPage::mixSampleRate() const {
        return m_mixSampleRate;
    }

    void SynthesisPage::setMixSampleRate(int value) {
        if (m_mixSampleRate != value) {
            m_mixSampleRate = value;
            Q_EMIT valuesChanged();
        }
    }

    int SynthesisPage::cacheMaximumGiB() const {
        return m_cacheMaximumGiB;
    }

    void SynthesisPage::setCacheMaximumGiB(int value) {
        if (m_cacheMaximumGiB != value) {
            m_cacheMaximumGiB = value;
            Q_EMIT valuesChanged();
        }
    }

    int SynthesisPage::cacheExpiryDays() const {
        return m_cacheExpiryDays;
    }

    void SynthesisPage::setCacheExpiryDays(int value) {
        if (m_cacheExpiryDays != value) {
            m_cacheExpiryDays = value;
            Q_EMIT valuesChanged();
        }
    }

    int SynthesisPage::audioDownloadMaximumMiB() const {
        return m_audioDownloadMaximumMiB;
    }

    void SynthesisPage::setAudioDownloadMaximumMiB(int value) {
        if (m_audioDownloadMaximumMiB != value) {
            m_audioDownloadMaximumMiB = value;
            Q_EMIT valuesChanged();
        }
    }

    int SynthesisPage::environmentTagTtlSeconds() const {
        return m_environmentTagTtlSeconds;
    }

    void SynthesisPage::setEnvironmentTagTtlSeconds(int value) {
        if (m_environmentTagTtlSeconds != value) {
            m_environmentTagTtlSeconds = value;
            Q_EMIT valuesChanged();
        }
    }

    QVariantMap SynthesisPage::cacheSizes() const {
        QVariantMap sizes{
            {QStringLiteral("pronunciation"), 0},
            {QStringLiteral("phoneme"), 0},
            {QStringLiteral("duration"), 0},
            {QStringLiteral("parameter"), 0},
            {QStringLiteral("audio"), 0},
        };
        auto interface = SynthInterface::instance();
        if (!interface || !interface->taskManager()) {
            return sizes;
        }
        auto manager = interface->taskManager();
        sizes.insert(QStringLiteral("pronunciation"), manager->cacheSize(SynthesisTaskType::Pronunciation));
        sizes.insert(QStringLiteral("phoneme"), manager->cacheSize(SynthesisTaskType::Phoneme));
        sizes.insert(QStringLiteral("duration"), manager->cacheSize(SynthesisTaskType::Duration));
        sizes.insert(QStringLiteral("parameter"), manager->cacheSize(SynthesisTaskType::Parameter));
        sizes.insert(QStringLiteral("audio"), manager->cacheSize(SynthesisTaskType::Audio));
        return sizes;
    }

    void SynthesisPage::clearCache(const QStringList &taskTypes) {
        auto interface = SynthInterface::instance();
        if (!interface || !interface->taskManager()) {
            return;
        }
        QList<SynthesisTaskType> types;
        if (taskTypes.contains(QStringLiteral("pronunciation"))) {
            types.append(SynthesisTaskType::Pronunciation);
        }
        if (taskTypes.contains(QStringLiteral("phoneme"))) {
            types.append(SynthesisTaskType::Phoneme);
        }
        if (taskTypes.contains(QStringLiteral("duration"))) {
            types.append(SynthesisTaskType::Duration);
        }
        if (taskTypes.contains(QStringLiteral("parameter"))) {
            types.append(SynthesisTaskType::Parameter);
        }
        if (taskTypes.contains(QStringLiteral("audio"))) {
            types.append(SynthesisTaskType::Audio);
        }
        if (!types.isEmpty()) {
            interface->taskManager()->clearCache(types);
        }
    }

}

#include "moc_SynthesisPage.cpp"
