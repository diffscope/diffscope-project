#include "NotificationCenter.h"

#include <QLoggingCategory>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/NotificationMessage.h>
#include <coreplugin/internal/NotificationManager.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcNotificationCenter, "diffscope.core.notificationcenter")

    static constexpr char kNotificationSettingsGroup[] = "Core::Internal::NotificationManager";

    NotificationCenter *NotificationCenter::m_instance = nullptr;

    NotificationCenter::NotificationCenter(QObject *parent) : QObject(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        loadHiddenMessageIdentifiers();
        m_globalNotificationManager = new NotificationManager(this, this);
    }

    NotificationCenter::~NotificationCenter() {
        m_instance = nullptr;
    }

    NotificationCenter *NotificationCenter::instance() {
        return m_instance;
    }

    NotificationManager *NotificationCenter::globalNotificationManager() const {
        return m_globalNotificationManager;
    }

    quint64 NotificationCenter::registerMessage(NotificationMessage *message) {
        if (const auto it = m_messageSequences.constFind(message); it != m_messageSequences.cend()) {
            return *it;
        }
        const auto sequence = ++m_nextMessageSequence;
        m_messageSequences.insert(message, sequence);
        connect(message, &QObject::destroyed, this, [this, message] {
            m_messageSequences.remove(message);
        });
        return sequence;
    }

    quint64 NotificationCenter::messageSequence(NotificationMessage *message) const {
        return m_messageSequences.value(message);
    }

    bool NotificationCenter::isMessageHidden(const QString &identifier) const {
        return m_hiddenMessageIdentifiers.contains(identifier);
    }

    void NotificationCenter::hideMessageIdentifier(const QString &identifier) {
        if (identifier.isEmpty() || m_hiddenMessageIdentifiers.contains(identifier)) {
            return;
        }
        qCInfo(lcNotificationCenter) << "Adding message identifier to hidden list:" << identifier;
        m_hiddenMessageIdentifiers.append(identifier);
        saveHiddenMessageIdentifiers();
    }

    void NotificationCenter::clearHiddenMessageIdentifiers() {
        qCInfo(lcNotificationCenter) << "Clearing hidden message identifiers";
        m_hiddenMessageIdentifiers.clear();
        saveHiddenMessageIdentifiers();
        for (auto *message : m_messageSequences.keys()) {
            if (message && !message->doNotShowAgainIdentifier().isEmpty()) {
                message->setAllowDoNotShowAgain(true);
            }
        }
    }

    void NotificationCenter::loadHiddenMessageIdentifiers() {
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(kNotificationSettingsGroup);
        m_hiddenMessageIdentifiers = settings->value("hiddenMessageIdentifiers", QStringList()).toStringList();
        settings->endGroup();
        qCInfo(lcNotificationCenter) << "Loaded hidden message identifiers:" << m_hiddenMessageIdentifiers;
    }

    void NotificationCenter::saveHiddenMessageIdentifiers() const {
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(kNotificationSettingsGroup);
        settings->setValue("hiddenMessageIdentifiers", m_hiddenMessageIdentifiers);
        settings->endGroup();
        qCInfo(lcNotificationCenter) << "Saved hidden message identifiers:" << m_hiddenMessageIdentifiers;
    }

}

#include "moc_NotificationCenter.cpp"
