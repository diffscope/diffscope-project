#include "SynthesisOptionsPage.h"

#include <QAbstractItemModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QSettings>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <synth/SynthInterface.h>
#include <synth/SynthesisTaskManager.h>
#include <synth/internal/ArchitectureExtraModel.h>

namespace Synth::Internal {

    SynthesisOptionsPage::SynthesisOptionsPage(QObject *parent)
        : Core::ISettingPage(QStringLiteral("org.diffscope.synth.Options"), parent),
          m_architectureExtras(new ArchitectureExtraModel(this)) {
        setTitle(tr("General"));
        setDescription(tr("Configure piece scheduling, cache and architecture-specific values"));
        connect(m_architectureExtras, &ArchitectureExtraModel::edited, this, &Core::ISettingPage::markDirty);
        connect(this, &SynthesisOptionsPage::valuesChanged, this, [this] {
            if (!m_loading)
                markDirty();
        });
    }

    SynthesisOptionsPage::~SynthesisOptionsPage() {
        delete m_widget;
    }

    QString SynthesisOptionsPage::sortKeyword() const {
        return QStringLiteral("00Synthesis");
    }

    bool SynthesisOptionsPage::matches(const QString &word) {
        if (Core::ISettingPage::matches(word))
            return true;
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool result{};
        return matcher && QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word) && result;
    }

    QObject *SynthesisOptionsPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("SynthesisOptionsPage"));
        if (component.isError())
            qFatal() << component.errorString();
        m_widget = component.createWithInitialProperties({
            {QStringLiteral("pageHandle"), QVariant::fromValue(this)},
            {QStringLiteral("architectureExtraModel"), QVariant::fromValue(m_architectureExtras)},
        });
        if (!m_widget)
            qFatal() << component.errorString();
        m_widget->setParent(this);
        return m_widget;
    }

    void SynthesisOptionsPage::beginSetting() {
        m_loading = true;
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synth"));
        m_paddingBase = settings->value(QStringLiteral("piecePaddingBaseMs"), 100.0).toDouble();
        m_paddingAdditional = settings->value(QStringLiteral("piecePaddingAdditionalMs"), 100.0).toDouble();
        m_paddingGap = settings->value(QStringLiteral("piecePaddingGapMs"), 1000.0).toDouble();
        m_restLyrics = settings->value(QStringLiteral("pieceRestLyrics"), QStringList{QStringLiteral("AP"), QStringLiteral("SP")})
                           .toStringList()
                           .join(QStringLiteral(", "));
        m_parameterSampleRate = settings->value(QStringLiteral("parameterSampleRate"), 100).toInt();
        m_mixSampleRate = settings->value(QStringLiteral("mixSampleRate"), 100).toInt();
        m_cacheMaximumGiB = static_cast<int>(settings->value(QStringLiteral("cacheMaximumBytes"), qint64(10) * 1024 * 1024 * 1024).toLongLong() / (qint64(1024) * 1024 * 1024));
        m_cacheExpiryDays = settings->value(QStringLiteral("cacheExpiryDays"), 30).toInt();
        m_audioDownloadMaximumMiB = static_cast<int>(settings->value(QStringLiteral("audioDownloadMaximumBytes"), qint64(512) * 1024 * 1024).toLongLong() / (qint64(1024) * 1024));
        m_environmentTagTtlSeconds = settings->value(QStringLiteral("environmentTagTtlSeconds"), 60).toInt();
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(
            settings->value(QStringLiteral("architectureExtras")).toByteArray(), &error
        );
        settings->endGroup();
        m_architectureExtras->setEntries(error.error == QJsonParseError::NoError && document.isObject() ? document.object() : QJsonObject{});
        setErrorMessage({});
        Q_EMIT valuesChanged();
        m_loading = false;
        Core::ISettingPage::beginSetting();
    }

    bool SynthesisOptionsPage::accept() {
        QJsonObject extras;
        QString error;
        if (!m_architectureExtras->entries(&extras, &error)) {
            setErrorMessage(error);
            return false;
        }
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
        settings->setValue(QStringLiteral("architectureExtras"), QJsonDocument(extras).toJson(QJsonDocument::Compact));
        settings->endGroup();
        if (auto interface = SynthInterface::instance(); interface && interface->taskManager())
            interface->taskManager()->reloadSettings();
        setErrorMessage({});
        return Core::ISettingPage::accept();
    }

    void SynthesisOptionsPage::endSetting() {
        setErrorMessage({});
        Core::ISettingPage::endSetting();
    }

    double SynthesisOptionsPage::paddingBase() const {
        return m_paddingBase;
    }
    void SynthesisOptionsPage::setPaddingBase(double value) {
        if (m_paddingBase != value) {
            m_paddingBase = value;
            Q_EMIT valuesChanged();
        }
    }
    double SynthesisOptionsPage::paddingAdditional() const {
        return m_paddingAdditional;
    }
    void SynthesisOptionsPage::setPaddingAdditional(double value) {
        if (m_paddingAdditional != value) {
            m_paddingAdditional = value;
            Q_EMIT valuesChanged();
        }
    }
    double SynthesisOptionsPage::paddingGap() const {
        return m_paddingGap;
    }
    void SynthesisOptionsPage::setPaddingGap(double value) {
        if (m_paddingGap != value) {
            m_paddingGap = value;
            Q_EMIT valuesChanged();
        }
    }
    QString SynthesisOptionsPage::restLyrics() const {
        return m_restLyrics;
    }
    void SynthesisOptionsPage::setRestLyrics(const QString &value) {
        if (m_restLyrics != value) {
            m_restLyrics = value;
            Q_EMIT valuesChanged();
        }
    }
    int SynthesisOptionsPage::parameterSampleRate() const {
        return m_parameterSampleRate;
    }
    void SynthesisOptionsPage::setParameterSampleRate(int value) {
        if (m_parameterSampleRate != value) {
            m_parameterSampleRate = value;
            Q_EMIT valuesChanged();
        }
    }
    int SynthesisOptionsPage::mixSampleRate() const {
        return m_mixSampleRate;
    }
    void SynthesisOptionsPage::setMixSampleRate(int value) {
        if (m_mixSampleRate != value) {
            m_mixSampleRate = value;
            Q_EMIT valuesChanged();
        }
    }
    int SynthesisOptionsPage::cacheMaximumGiB() const {
        return m_cacheMaximumGiB;
    }
    void SynthesisOptionsPage::setCacheMaximumGiB(int value) {
        if (m_cacheMaximumGiB != value) {
            m_cacheMaximumGiB = value;
            Q_EMIT valuesChanged();
        }
    }
    int SynthesisOptionsPage::cacheExpiryDays() const {
        return m_cacheExpiryDays;
    }
    void SynthesisOptionsPage::setCacheExpiryDays(int value) {
        if (m_cacheExpiryDays != value) {
            m_cacheExpiryDays = value;
            Q_EMIT valuesChanged();
        }
    }
    int SynthesisOptionsPage::audioDownloadMaximumMiB() const {
        return m_audioDownloadMaximumMiB;
    }
    void SynthesisOptionsPage::setAudioDownloadMaximumMiB(int value) {
        if (m_audioDownloadMaximumMiB != value) {
            m_audioDownloadMaximumMiB = value;
            Q_EMIT valuesChanged();
        }
    }
    int SynthesisOptionsPage::environmentTagTtlSeconds() const {
        return m_environmentTagTtlSeconds;
    }
    void SynthesisOptionsPage::setEnvironmentTagTtlSeconds(int value) {
        if (m_environmentTagTtlSeconds != value) {
            m_environmentTagTtlSeconds = value;
            Q_EMIT valuesChanged();
        }
    }

    QAbstractItemModel *SynthesisOptionsPage::architectureExtraModel() const {
        return m_architectureExtras;
    }
    QString SynthesisOptionsPage::errorMessage() const {
        return m_errorMessage;
    }

    void SynthesisOptionsPage::clearCache() {
        if (auto interface = SynthInterface::instance(); interface && interface->taskManager())
            interface->taskManager()->clearCache();
    }

    void SynthesisOptionsPage::setErrorMessage(const QString &message) {
        if (m_errorMessage == message)
            return;
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }

}

#include "moc_SynthesisOptionsPage.cpp"
