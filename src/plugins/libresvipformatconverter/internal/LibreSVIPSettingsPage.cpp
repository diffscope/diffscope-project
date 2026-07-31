#include "LibreSVIPSettingsPage.h"

#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <libresvipformatconverter/internal/LibreSVIPManager.h>

namespace LibreSVIPFormatConverter::Internal {

    LibreSVIPSettingsPage::LibreSVIPSettingsPage(QObject *parent)
        : ISettingPage(QString::fromLatin1(LibreSVIPManager::settingsPageId()), parent) {
        setTitle(tr("LibreSVIP Format Conversion"));
        setDescription(tr("Configure and manage the LibreSVIP command-line format converter"));
        auto manager = LibreSVIPManager::instance();
        connect(manager, &LibreSVIPManager::configurationChanged, this, [this] {
            Q_EMIT executablePathChanged();
            Q_EMIT downloadedInstallationExistsChanged();
        });
    }

    LibreSVIPSettingsPage::~LibreSVIPSettingsPage() {
        delete m_widget;
    }

    QString LibreSVIPSettingsPage::executablePath() const {
        return LibreSVIPManager::instance()->executablePath();
    }

    bool LibreSVIPSettingsPage::downloadedInstallationExists() const {
        return LibreSVIPManager::instance()->downloadedInstallationExists();
    }

    QString LibreSVIPSettingsPage::sortKeyword() const {
        return QStringLiteral("LibreSVIP");
    }

    bool LibreSVIPSettingsPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QObject *LibreSVIPSettingsPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.LibreSVIPFormatConverter", "LibreSVIPSettingsPage");
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

    void LibreSVIPSettingsPage::browse() {
        LibreSVIPManager::instance()->browseAndConfigure(pageWindow());
    }

    void LibreSVIPSettingsPage::download() {
        LibreSVIPManager::instance()->downloadAndConfigure(pageWindow());
    }

    void LibreSVIPSettingsPage::removeDownloadedInstallation() {
        LibreSVIPManager::instance()->removeDownloadedInstallation(pageWindow());
    }

    bool LibreSVIPSettingsPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool matches = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(matches), word);
        return matches;
    }

    QWindow *LibreSVIPSettingsPage::pageWindow() const {
        auto item = qobject_cast<QQuickItem *>(m_widget);
        return item ? item->window() : nullptr;
    }

}
